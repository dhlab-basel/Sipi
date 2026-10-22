/*
 * Copyright © 2016 - 2026 Swiss National Data and Service Center for the Humanities and/or DaSCH Service Platform
 * contributors. SPDX-License-Identifier: AGPL-3.0-or-later
 */

// Co-located unit tests (ADR-0003) for the transport-pure IIIF response builder,
// build_image_response. Because it takes only (request, engine context, cancel
// predicate) it runs without a socket: the body shape (FileBody / StreamBody /
// EmptyBody) and the pre-commit status codes are checked directly.

#include "gtest/gtest.h"

#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "cache/SipiCache.h"
#include "ffi/engine_context.h"
#include "ffi/serve_image.h"
#include "ffi/serve_response.h"
#include "ffi/sipi_ffi.h"
#include "image/SipiImage.h"
#include "observability/metrics.h"
#include "test_paths.h"

namespace {

using namespace Sipi::ffi;

const std::string kImagesDir = sipi::test::data_dir() + "/images";

// Resolve the fixture through realpath(), matching what the real serve path
// hands the FFI (R2 path validation realpath()s every request). It also sidesteps
// a Bazel-runfiles artifact: the fixture is a symlink, and libmagic would sniff
// the *symlink* (inode/symlink) rather than the TIFF behind it.
std::string fixture(const std::string &rel)
{
  const std::string p = kImagesDir + rel;
  char buf[PATH_MAX];
  return realpath(p.c_str(), buf) != nullptr ? std::string(buf) : p;
}

const auto kNeverCancelled = std::function<bool()>([] { return false; });

// A FULL-region / FULL-size / no-rotation request for the given output format.
SipiIiifParams full_params(SipiFormatType format, SipiQualityType quality = SIPI_QUALITY_DEFAULT)
{
  SipiIiifParams p{};
  p.region_type = SIPI_REGION_FULL;
  p.size_type = SIPI_SIZE_FULL;
  p.rotation = 0.F;
  p.rotation_mirror = 0;
  p.quality_type = quality;
  p.format_type = format;
  return p;
}

EngineContext bare_engine()
{
  EngineContext eng;// all services null (cache/budget disabled)
  eng.imgroot = kImagesDir;
  eng.resolved_imgroot = kImagesDir;
  eng.jpeg_quality = 60;
  return eng;
}

SipiServeRequest make_request(const std::string &path, const SipiIiifParams &params, int is_head = 0)
{
  SipiServeRequest req{};
  req.resolved_path = path.c_str();
  req.prefix = "unit";
  req.identifier = "lena512.tif";
  req.params = params;
  req.restricted_width = 0;
  req.restricted_height = 0;
  req.watermark_path = nullptr;
  req.forwarded_proto = nullptr;
  req.forwarded_host = "localhost";
  req.request_uri = "/unit/lena512.tif";
  req.is_head = is_head;
  return req;
}

bool has_header(const ServeResponse &r, const std::string &name, const std::string &value)
{
  for (const auto &[n, v] : r.headers) {
    if (n == name && v == value) { return true; }
  }
  return false;
}

// Collects a reported SipiImageErrorReport's fields, copying the strings —
// the struct's fields are only valid for the report_error call's duration.
struct ReportedError
{
  bool fired = false;
  std::string phase;
  std::string message;
  std::string input_file;
};

void collect_report(void *ctx, const SipiImageErrorReport *err)
{
  auto *out = static_cast<ReportedError *>(ctx);
  out->fired = true;
  out->phase = err->phase != nullptr ? err->phase : "";
  out->message = err->message != nullptr ? err->message : "";
  out->input_file = err->input_file != nullptr ? err->input_file : "";
}


// Region / size setters for the flat seam params.
void set_coords(SipiIiifParams &p, float x, float y, float w, float h)
{
  p.region_type = SIPI_REGION_COORDS;
  p.region[0] = x;
  p.region[1] = y;
  p.region[2] = w;
  p.region[3] = h;
}

void set_pct_region(SipiIiifParams &p, float x, float y, float w, float h)
{
  set_coords(p, x, y, w, h);
  p.region_type = SIPI_REGION_PERCENTS;
}

void set_size(SipiIiifParams &p, SipiSizeType type, size_t nx, size_t ny, bool upscaling = false)
{
  p.size_type = type;
  p.size_nx = nx;
  p.size_ny = ny;
  p.size_upscaling = upscaling ? 1 : 0;
}

// A unique scratch directory under the Bazel per-test tmp dir.
std::string scratch_dir(const std::string &name)
{
  const std::string dir = sipi::test::tmp_dir() + "/serve_image_test_" + name;
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

// Runs the StreamBody producer into memory, then the response's on_complete.
std::vector<std::uint8_t> drain(ServeResponse &resp)
{
  std::vector<std::uint8_t> bytes;
  SipiResponse sink_resp{};
  sink_resp.ctx = &bytes;
  sink_resp.write = [](void *ctx, const std::uint8_t *data, size_t len) -> int {
    auto *out = static_cast<std::vector<std::uint8_t> *>(ctx);
    out->insert(out->end(), data, data + len);
    return 0;
  };
  const StreamSink sink(sink_resp);
  const int rc = std::get<StreamBody>(resp.body).producer->produce(sink);
  EXPECT_EQ(rc, 0);
  if (resp.on_complete) { resp.on_complete(); }
  return bytes;
}

// Drains a StreamBody response and decodes the encoded image.
std::unique_ptr<Sipi::SipiImage> decode_body(ServeResponse &resp, const std::string &ext)
{
  const auto bytes = drain(resp);
  const std::string path = scratch_dir("decode") + "/body." + ext;
  {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  auto img = std::make_unique<Sipi::SipiImage>();
  const auto r = img->read(path);
  EXPECT_TRUE(r.has_value()) << path;
  return img;
}

// Serves `params` for `path` under the restriction (rw, rh) and decodes the
// PNG body; nullptr when the request is refused.
std::unique_ptr<Sipi::SipiImage> serve_png(const std::string &path,
  SipiIiifParams params,
  uint32_t rw,
  uint32_t rh,
  const EngineContext &eng)
{
  params.format_type = SIPI_FORMAT_PNG;
  auto req = make_request(path, params);
  req.restricted_width = rw;
  req.restricted_height = rh;
  auto result = build_image_response(req, eng, kNeverCancelled);
  if (!result.has_value()) { return nullptr; }
  EXPECT_EQ(result->http_status, 200);
  return decode_body(*result, "png");
}

// The pixels of `img` inside the rectangle (x, y, w, h).
std::vector<Sipi::byte> crop_pixels(const Sipi::SipiImage &img, size_t x, size_t y, size_t w, size_t h)
{
  const size_t nc = img.getNc();
  const size_t elem = img.getBps() == 16 ? 2 : 1;
  const auto px = img.pixels_view();
  std::vector<Sipi::byte> out;
  for (size_t j = y; j < y + h; ++j) {
    const auto row = px.begin() + static_cast<std::ptrdiff_t>((j * img.getNx() + x) * nc * elem);
    out.insert(out.end(), row, row + static_cast<std::ptrdiff_t>(w * nc * elem));
  }
  return out;
}

std::vector<Sipi::byte> all_pixels(const Sipi::SipiImage &img)
{
  return { img.pixels_view().begin(), img.pixels_view().end() };
}
}// namespace

TEST(BuildImageResponse, MissingFileIsNotFound)
{
  const std::string path = kImagesDir + "/unit/does_not_exist.tif";
  const auto params = full_params(SIPI_FORMAT_TIF);
  const auto req = make_request(path, params);

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SipiStatus::NotFound);
}

TEST(BuildImageResponse, DirectPassthroughIsFileBody)
{
  // Same format as the source (TIFF), full region + size, no transform → the
  // engine serves the file directly (sendFile), not a re-encode.
  const std::string path = fixture("/unit/lena512.tif");
  const auto params = full_params(SIPI_FORMAT_TIF);
  const auto req = make_request(path, params);

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->http_status, 200);
  EXPECT_TRUE(std::holds_alternative<FileBody>(result->body));
  EXPECT_EQ(std::get<FileBody>(result->body).path, path);
  EXPECT_TRUE(has_header(*result, "Content-Type", "image/tiff"));
}

TEST(BuildImageResponse, FormatConversionIsStreamBody)
{
  // A different output format (PNG) forces a decode + re-encode: the body is a
  // streamed producer, not a file passthrough.
  const std::string path = fixture("/unit/lena512.tif");
  const auto params = full_params(SIPI_FORMAT_PNG);
  const auto req = make_request(path, params);

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->http_status, 200);
  EXPECT_TRUE(std::holds_alternative<StreamBody>(result->body));
  EXPECT_TRUE(has_header(*result, "Content-Type", "image/png"));
}

TEST(BuildImageResponse, HeadRequestIsEmptyBody)
{
  const std::string path = fixture("/unit/lena512.tif");
  const auto params = full_params(SIPI_FORMAT_PNG);
  const auto req = make_request(path, params, /*is_head=*/1);

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->http_status, 200);
  EXPECT_TRUE(std::holds_alternative<EmptyBody>(result->body));
  EXPECT_TRUE(has_header(*result, "Content-Type", "image/png"));
}

TEST(BuildImageResponse, ClientGoneBeforeDecode)
{
  const std::string path = fixture("/unit/lena512.tif");
  const auto params = full_params(SIPI_FORMAT_PNG);// decode path (polls cancel)
  const auto req = make_request(path, params);

  const auto always_cancelled = std::function<bool()>([] { return true; });
  const auto result = build_image_response(req, bare_engine(), always_cancelled);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SipiStatus::ClientGone);
}

// The restricted dims (RW, RH) arrive decided by the shell. lena512.tif is
// 512x512; the in-bounds case is the canary that the two refusals below come
// from the restricted pair, not from the request.
TEST(BuildImageResponse, RestrictedDimsWithinImageServe)
{
  const std::string path = fixture("/unit/lena512.tif");
  auto req = make_request(path, full_params(SIPI_FORMAT_PNG));
  req.restricted_width = 128;
  req.restricted_height = 128;

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->http_status, 200);
}

TEST(BuildImageResponse, HalfSetRestrictedPairIsInternalError)
{
  const std::string path = fixture("/unit/lena512.tif");
  for (const auto &[rw, rh] : { std::pair<uint32_t, uint32_t>{ 128, 0 }, std::pair<uint32_t, uint32_t>{ 0, 128 } }) {
    auto req = make_request(path, full_params(SIPI_FORMAT_PNG));
    req.restricted_width = rw;
    req.restricted_height = rh;

    const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
    ASSERT_FALSE(result.has_value()) << rw << "x" << rh;
    EXPECT_EQ(result.error(), SipiStatus::InternalError) << rw << "x" << rh;
  }
}

// Checked before the HEAD return, so HEAD and GET agree.
TEST(BuildImageResponse, RestrictedAxisBelowTwoIsInternalError)
{
  const std::string path = fixture("/unit/lena512.tif");
  for (const auto &[rw, rh] : { std::pair<uint32_t, uint32_t>{ 1, 128 }, std::pair<uint32_t, uint32_t>{ 128, 1 } }) {
    for (const int head : { 0, 1 }) {
      auto req = make_request(path, full_params(SIPI_FORMAT_PNG));
      req.restricted_width = rw;
      req.restricted_height = rh;
      req.is_head = head;

      const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
      ASSERT_FALSE(result.has_value()) << rw << "x" << rh << " head=" << head;
      EXPECT_EQ(result.error(), SipiStatus::InternalError) << rw << "x" << rh << " head=" << head;
    }
  }
}

TEST(BuildImageResponse, RestrictedDimsLargerThanImageIsInternalError)
{
  const std::string path = fixture("/unit/lena512.tif");
  for (const auto &[rw, rh] : { std::pair<uint32_t, uint32_t>{ 513, 512 }, std::pair<uint32_t, uint32_t>{ 512, 513 } }) {
    auto req = make_request(path, full_params(SIPI_FORMAT_PNG));
    req.restricted_width = rw;
    req.restricted_height = rh;

    const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
    ASSERT_FALSE(result.has_value()) << rw << "x" << rh;
    EXPECT_EQ(result.error(), SipiStatus::InternalError) << rw << "x" << rh;
  }
}

// A size-restricted image is served as its own image of (RW, RH): region and
// size resolve against (RW, RH), and every output is cut from the one
// rendering at (RW, RH). odd_373x496.jp2 is 373x496; (96, 128) divides
// neither axis.
namespace {
const std::string kOddJp2 = "/unit/odd_373x496.jp2";
constexpr uint32_t kRw = 96;
constexpr uint32_t kRh = 128;

SipiIiifParams png_params() { return full_params(SIPI_FORMAT_PNG); }
}// namespace

TEST(RestrictedImage, FullMaxIsExactlyTheRestrictedDims)
{
  const auto jp2 = serve_png(fixture(kOddJp2), png_params(), kRw, kRh, bare_engine());
  ASSERT_NE(jp2, nullptr);
  EXPECT_EQ(jp2->getNx(), kRw);
  EXPECT_EQ(jp2->getNy(), kRh);

  const auto tif = serve_png(fixture("/unit/lena512.tif"), png_params(), 100, 77, bare_engine());
  ASSERT_NE(tif, nullptr);
  EXPECT_EQ(tif->getNx(), 100U);
  EXPECT_EQ(tif->getNy(), 77U);
}

TEST(RestrictedImage, SourceFormatFullMaxIsStreamedNeverTheFile)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  auto req = make_request(odd_jp2, full_params(SIPI_FORMAT_JP2));
  req.restricted_width = kRw;
  req.restricted_height = kRh;

  auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<StreamBody>(result->body));
  const auto img = decode_body(*result, "jp2");
  EXPECT_EQ(img->getNx(), kRw);
  EXPECT_EQ(img->getNy(), kRh);
}

TEST(RestrictedImage, RegionIsTheMatchingCropOfFullMax)
{
  const auto eng = bare_engine();
  const auto full = serve_png(fixture(kOddJp2), png_params(), kRw, kRh, eng);
  ASSERT_NE(full, nullptr);

  auto coords = png_params();
  set_coords(coords, 10, 20, 40, 50);
  const auto region = serve_png(fixture(kOddJp2), coords, kRw, kRh, eng);
  ASSERT_NE(region, nullptr);
  ASSERT_EQ(region->getNx(), 40U);
  ASSERT_EQ(region->getNy(), 50U);
  EXPECT_EQ(all_pixels(*region), crop_pixels(*full, 10, 20, 40, 50));

  auto quarter = png_params();
  set_pct_region(quarter, 0, 0, 50, 50);
  const auto top_left = serve_png(fixture(kOddJp2), quarter, kRw, kRh, eng);
  ASSERT_NE(top_left, nullptr);
  ASSERT_EQ(top_left->getNx(), kRw / 2);
  ASSERT_EQ(top_left->getNy(), kRh / 2);
  EXPECT_EQ(all_pixels(*top_left), crop_pixels(*full, 0, 0, kRw / 2, kRh / 2));

  auto outside = png_params();
  set_coords(outside, 80, 100, 40, 40);
  const auto clamped = serve_png(fixture(kOddJp2), outside, kRw, kRh, eng);
  ASSERT_NE(clamped, nullptr);
  ASSERT_EQ(clamped->getNx(), 16U);
  ASSERT_EQ(clamped->getNy(), 28U);
  EXPECT_EQ(all_pixels(*clamped), crop_pixels(*full, 80, 100, 16, 28));
}

TEST(RestrictedImage, RegionStartingBeyondTheRestrictedDimsIsBadRequest)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  for (const auto &[x, y] : { std::pair<float, float>{ kRw, 0 }, std::pair<float, float>{ 0, kRh } }) {
    auto params = png_params();
    set_coords(params, x, y, 10, 10);
    auto req = make_request(odd_jp2, params);
    req.restricted_width = kRw;
    req.restricted_height = kRh;
    const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
    ASSERT_FALSE(result.has_value()) << x << "," << y;
    EXPECT_EQ(result.error(), SipiStatus::BadRequest) << x << "," << y;
  }
}

namespace {
struct RestrictedSizeCase
{
  const char *name;
  bool quarter_region;
  SipiSizeType type;
  size_t nx;
  size_t ny;
  float percent;
  bool upscaling;
  size_t want_w;// 0 = refused with 400
  size_t want_h;
};

class RestrictedSize : public ::testing::TestWithParam<RestrictedSizeCase>
{};
}// namespace

TEST_P(RestrictedSize, ResolvesAgainstTheRegionAndNeverExceedsFullMax)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  const auto &c = GetParam();
  auto params = png_params();
  if (c.quarter_region) { set_pct_region(params, 0, 0, 50, 50); }
  set_size(params, c.type, c.nx, c.ny, c.upscaling);
  params.size_percent = c.percent;

  auto req = make_request(odd_jp2, params);
  req.restricted_width = kRw;
  req.restricted_height = kRh;
  auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  if (c.want_w == 0) {
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SipiStatus::BadRequest);
    return;
  }
  ASSERT_TRUE(result.has_value());
  const auto img = decode_body(*result, "png");
  EXPECT_EQ(img->getNx(), c.want_w);
  EXPECT_EQ(img->getNy(), c.want_h);
}

INSTANTIATE_TEST_SUITE_P(Sizes,
  RestrictedSize,
  ::testing::Values(RestrictedSizeCase{ "max_quarter", true, SIPI_SIZE_FULL, 0, 0, 0.F, false, 48, 64 },
    RestrictedSizeCase{ "upmax_quarter", true, SIPI_SIZE_FULL, 0, 0, 0.F, true, 48, 64 },
    RestrictedSizeCase{ "upmax_full", false, SIPI_SIZE_FULL, 0, 0, 0.F, true, kRw, kRh },
    RestrictedSizeCase{ "box_40", false, SIPI_SIZE_MAXDIM, 40, 40, 0.F, false, 30, 40 },
    RestrictedSizeCase{ "uppct200_quarter", true, SIPI_SIZE_PERCENTS, 0, 0, 200.F, true, kRw, kRh },
    RestrictedSizeCase{ "uppct200_full", false, SIPI_SIZE_PERCENTS, 0, 0, 200.F, true, 0, 0 },
    RestrictedSizeCase{ "pct200_quarter", true, SIPI_SIZE_PERCENTS, 0, 0, 200.F, false, 0, 0 },
    RestrictedSizeCase{ "upw_wider", false, SIPI_SIZE_PIXELS_X, kRw + 1, 0, 0.F, true, 0, 0 },
    RestrictedSizeCase{ "uph_taller", false, SIPI_SIZE_PIXELS_Y, 0, kRh + 1, 0.F, true, 0, 0 },
    RestrictedSizeCase{ "upwh_wider", false, SIPI_SIZE_PIXELS_XY, kRw + 1, 10, 0.F, true, 0, 0 },
    RestrictedSizeCase{ "upwh_taller", false, SIPI_SIZE_PIXELS_XY, 10, kRh + 1, 0.F, true, 0, 0 },
    RestrictedSizeCase{ "upbox_beyond", false, SIPI_SIZE_MAXDIM, 200, 200, 0.F, true, 0, 0 }),
  [](const ::testing::TestParamInfo<RestrictedSizeCase> &info) { return std::string(info.param.name); });

TEST(RestrictedImage, EdgeTileAtSizeMaxIsAPureCrop)
{
  auto params = png_params();
  set_coords(params, 64, kRh - 1, 32, 1);
  const auto tile = serve_png(fixture(kOddJp2), params, kRw, kRh, bare_engine());
  ASSERT_NE(tile, nullptr);
  ASSERT_EQ(tile->getNx(), 32U);
  ASSERT_EQ(tile->getNy(), 1U);
  const auto full = serve_png(fixture(kOddJp2), png_params(), kRw, kRh, bare_engine());
  ASSERT_NE(full, nullptr);
  EXPECT_EQ(all_pixels(*tile), crop_pixels(*full, 64, kRh - 1, 32, 1));
}

// A 1-sample axis cannot be scaled, so a request that would need it is a 400
// on GET and HEAD alike, never a 500 from the scale operators.
TEST(RestrictedImage, UnscalableOneSampleAxisIsBadRequestOnGetAndHead)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  auto to_one_row = png_params();
  set_coords(to_one_row, 0, 0, 40, 60);
  set_size(to_one_row, SIPI_SIZE_PIXELS_Y, 0, 1);
  auto from_one_column = png_params();
  set_coords(from_one_column, kRw - 1, 0, 40, kRh);// clamps to 1 x RH
  set_size(from_one_column, SIPI_SIZE_PIXELS_XY, 2, 100, true);
  for (const auto &params : { to_one_row, from_one_column }) {
    for (const int is_head : { 0, 1 }) {
      auto req = make_request(odd_jp2, params, is_head);
      req.restricted_width = kRw;
      req.restricted_height = kRh;
      const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
      ASSERT_FALSE(result.has_value()) << "head=" << is_head;
      EXPECT_EQ(result.error(), SipiStatus::BadRequest) << "head=" << is_head;
    }
  }
}

TEST(RestrictedImage, BoundIsCheckedBeforeRotation)
{
  auto full = png_params();
  full.rotation = 90.F;
  const auto rotated = serve_png(fixture(kOddJp2), full, kRw, kRh, bare_engine());
  ASSERT_NE(rotated, nullptr);
  EXPECT_EQ(rotated->getNx(), kRh);
  EXPECT_EQ(rotated->getNy(), kRw);

  auto region = png_params();
  set_coords(region, 0, 0, 40, 60);
  region.rotation = 90.F;
  const auto rotated_region = serve_png(fixture(kOddJp2), region, kRw, kRh, bare_engine());
  ASSERT_NE(rotated_region, nullptr);
  EXPECT_EQ(rotated_region->getNx(), 60U);
  EXPECT_EQ(rotated_region->getNy(), 40U);
}

TEST(RestrictedImage, SizeAndWatermarkServeTheRestrictedDimsWatermarked)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  const std::string watermark = fixture("/unit/watermark_correct.tif");
  const auto plain = serve_png(fixture(kOddJp2), png_params(), kRw, kRh, bare_engine());
  ASSERT_NE(plain, nullptr);

  auto params = png_params();
  auto req = make_request(odd_jp2, params);
  req.restricted_width = kRw;
  req.restricted_height = kRh;
  req.watermark_path = watermark.c_str();
  auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  const auto marked = decode_body(*result, "png");
  ASSERT_EQ(marked->getNx(), kRw);
  ASSERT_EQ(marked->getNy(), kRh);
  EXPECT_NE(all_pixels(*marked), all_pixels(*plain));
}

TEST(RestrictedImage, WatermarkOnlyKeepsNativeDimsAndCoordinates)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  const std::string watermark = fixture("/unit/watermark_correct.tif");
  auto params = png_params();
  set_coords(params, 0, 0, 200, 300);
  auto req = make_request(odd_jp2, params);
  req.watermark_path = watermark.c_str();
  auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_TRUE(result.has_value());
  const auto img = decode_body(*result, "png");
  EXPECT_EQ(img->getNx(), 200U);
  EXPECT_EQ(img->getNy(), 300U);
}

TEST(RestrictedImage, SizeAndWatermarkNeverReachThePassthrough)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  const std::string watermark = fixture("/unit/watermark_correct.tif");
  // (373, 496) is the native size: an image already inside the box.
  for (const auto &[rw, rh] : { std::pair<uint32_t, uint32_t>{ kRw, kRh }, std::pair<uint32_t, uint32_t>{ 373, 496 } }) {
    for (const char *wm : { static_cast<const char *>(nullptr), watermark.c_str() }) {
      auto req = make_request(odd_jp2, full_params(SIPI_FORMAT_JP2));
      req.restricted_width = rw;
      req.restricted_height = rh;
      req.watermark_path = wm;
      const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
      ASSERT_TRUE(result.has_value()) << rw << "x" << rh;
      EXPECT_TRUE(std::holds_alternative<StreamBody>(result->body)) << rw << "x" << rh << " wm=" << (wm != nullptr);
    }
  }
}

// A restricted and an unrestricted request with the same canonical
// coordinates, and two restrictions of the same file, must never share a
// cache entry: a cache hit would hand one view's pixels to the other.
TEST(RestrictedImage, CacheKeySeparatesRestrictions)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  const std::string dir = scratch_dir("cache");
  Sipi::SipiCache cache(dir + "/cache");
  auto eng = bare_engine();
  eng.cache = &cache;

  auto params = png_params();
  set_coords(params, 0, 0, 40, 40);
  set_size(params, SIPI_SIZE_PIXELS_XY, 10, 10);

  const auto serve = [&](uint32_t rw, uint32_t rh) {
    auto req = make_request(odd_jp2, params);
    req.restricted_width = rw;
    req.restricted_height = rh;
    auto result = build_image_response(req, eng, kNeverCancelled);
    ASSERT_TRUE(result.has_value()) << rw << "x" << rh;
    ASSERT_TRUE(std::holds_alternative<StreamBody>(result->body)) << rw << "x" << rh << " was a cache hit";
    drain(*result);
  };

  serve(0, 0);
  serve(kRw, kRh);
  EXPECT_EQ(cache.getNfiles(), 2U);
  serve(kRh, kRw);
  EXPECT_EQ(cache.getNfiles(), 3U);
}

// The size in the Cache key resolves against the region extent: `pct:50`,
// `50,` and `50,67` on an 80x100 region are 40x50, 50x63 and 50x67, three
// images, three entries, restricted or not.
TEST(BuildImageResponse, CacheKeyResolvesTheSizeAgainstTheRegion)
{
  const std::string odd_jp2 = fixture(kOddJp2);
  for (const auto &[rw, rh] : { std::pair<uint32_t, uint32_t>{ 0, 0 }, std::pair<uint32_t, uint32_t>{ kRw, kRh } }) {
    const std::string dir = scratch_dir("cache_region_size_" + std::to_string(rw));
    Sipi::SipiCache cache(dir + "/cache");
    auto eng = bare_engine();
    eng.cache = &cache;

    auto pct = png_params();
    set_coords(pct, 0, 0, 80, 100);
    pct.size_type = SIPI_SIZE_PERCENTS;
    pct.size_percent = 50.F;
    auto width = png_params();
    set_coords(width, 0, 0, 80, 100);
    set_size(width, SIPI_SIZE_PIXELS_X, 50, 0);
    auto both = png_params();
    set_coords(both, 0, 0, 80, 100);
    set_size(both, SIPI_SIZE_PIXELS_XY, 50, 67);

    for (const auto &[params, w, h] : { std::tuple{ pct, size_t{ 40 }, size_t{ 50 } },
           std::tuple{ width, size_t{ 50 }, size_t{ 63 } },
           std::tuple{ both, size_t{ 50 }, size_t{ 67 } } }) {
      auto req = make_request(odd_jp2, params);
      req.restricted_width = rw;
      req.restricted_height = rh;
      auto result = build_image_response(req, eng, kNeverCancelled);
      ASSERT_TRUE(result.has_value());
      ASSERT_TRUE(std::holds_alternative<StreamBody>(result->body)) << w << "x" << h << " was a cache hit, rw=" << rw;
      const auto img = decode_body(*result, "png");
      EXPECT_EQ(img->getNx(), w);
      EXPECT_EQ(img->getNy(), h);
    }
    EXPECT_EQ(cache.getNfiles(), 3U) << "rw=" << rw;
  }
}

TEST(BuildImageResponse, ReportErrorFiresOnReadFailure)
{
  // test.csv exists but is not a recognised image format — read_shape()
  // throws before any decode is attempted, exercising the seam's earliest
  // report_error call site.
  const std::string path = fixture("/unit/test.csv");
  const auto params = full_params(SIPI_FORMAT_TIF);
  auto req = make_request(path, params);
  ReportedError out;
  req.report_error = collect_report;
  req.report_ctx = &out;

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SipiStatus::InternalError);
  EXPECT_TRUE(out.fired);
  EXPECT_EQ(out.phase, "read");
  EXPECT_FALSE(out.message.empty());
  EXPECT_EQ(out.input_file, path);
}

TEST(BuildImageResponse, ReportErrorNullCallbackIsSafeNoOp)
{
  // Same failing request as above, but report_error is left null (make_request's
  // SipiServeRequest{} default) — the seam's "NULL = absent" idiom must not
  // crash or otherwise change the returned status.
  const std::string path = fixture("/unit/test.csv");
  const auto params = full_params(SIPI_FORMAT_TIF);
  const auto req = make_request(path, params);

  const auto result = build_image_response(req, bare_engine(), kNeverCancelled);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SipiStatus::InternalError);
}

TEST(StatusFor, ReportPolicyCodeIsInternalError)
{
  const Sipi::SipiValueError err(Sipi::ErrorCode::kDecodeFailed, "boom");
  EXPECT_EQ(status_for(err), SipiStatus::InternalError);
}

TEST(StatusFor, SkipPolicyCodeIsAlsoInternalError)
{
  // Every ErrorCode maps to kInternalError today (error-model.md); the
  // sentry_policy (kReport vs kSkip) is orthogonal to the HTTP status.
  const Sipi::SipiValueError err(Sipi::ErrorCode::kClientAbort, "client gone");
  EXPECT_EQ(status_for(err), SipiStatus::InternalError);
}

TEST(ReportValueError, CallsCallbackForReportPolicyCode)
{
  const Sipi::SipiValueError err(Sipi::ErrorCode::kDecodeFailed, "decode boom");
  Sipi::observability::ImageContext ctx;
  ctx.input_file = "/unit/lena512.tif";
  ctx.output_format = "png";
  ReportedError out;

  report_value_error(collect_report, &out, err, "convert", ctx);

  ASSERT_TRUE(out.fired);
  EXPECT_EQ(out.phase, "convert");
  EXPECT_EQ(out.message, err.diagnostic_message());
  EXPECT_EQ(out.input_file, ctx.input_file);
}

TEST(ReportValueError, SkipsCallbackForClientAbort)
{
  // The load-bearing case: kSkip must not call the callback at all,
  // not merely pass it an empty report.
  const Sipi::SipiValueError err(Sipi::ErrorCode::kClientAbort, "client gone");
  Sipi::observability::ImageContext ctx;
  ReportedError out;

  report_value_error(collect_report, &out, err, "write", ctx);

  EXPECT_FALSE(out.fired);
}

TEST(ReportValueError, NullCallbackIsSafeNoOp)
{
  const Sipi::SipiValueError err(Sipi::ErrorCode::kDecodeFailed, "decode boom");
  Sipi::observability::ImageContext ctx;

  report_value_error(nullptr, nullptr, err, "read", ctx);
}

// A wedged Kakadu decode (DEV-7080) must be bounded by the seam's wall-clock
// deadline instead of hanging the calling thread forever. Both committed
// reproducers wedge inside read_shape, so a short deadline must trip well
// under the 30s bound this test enforces (comfortably below the production
// default of 120s).
class DecodeHangWatchdog : public ::testing::TestWithParam<const char *>
{};

TEST_P(DecodeHangWatchdog, HitsDeadlineAndCountsWedgedThread)
{
  const std::string rel = GetParam();
  const std::string basename = rel.substr(rel.find_last_of('/') + 1);
  const std::string path = fixture(rel);
  const auto params = full_params(SIPI_FORMAT_TIF);
  auto req = make_request(path, params);
  req.identifier = basename.c_str();

  auto eng = bare_engine();
  eng.decode_timeout_ms = 2000;

  const auto before = Sipi::observability::Metrics::instance().wedged_threads.Value();
  const auto start = std::chrono::steady_clock::now();
  const auto result = build_image_response(req, eng, kNeverCancelled);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SipiStatus::InternalError);
  EXPECT_LT(elapsed, std::chrono::seconds(30));
  EXPECT_EQ(Sipi::observability::Metrics::instance().wedged_threads.Value(), before + 1);
}

INSTANTIATE_TEST_SUITE_P(Jp2HangFixtures,
  DecodeHangWatchdog,
  ::testing::Values("/hang/dev7080_read_shape_hang.jp2", "/hang/nightly_j2k_read_shape_hang.jp2"));

TEST(BuildImageResponse, GoodJp2DecodesWithinDeadline)
{
  const std::string path = fixture("/unit/lena512.jp2");
  const auto params = full_params(SIPI_FORMAT_TIF);
  auto req = make_request(path, params);
  req.identifier = "lena512.jp2";

  auto eng = bare_engine();
  eng.decode_timeout_ms = 120000;

  const auto before = Sipi::observability::Metrics::instance().wedged_threads.Value();
  const auto result = build_image_response(req, eng, kNeverCancelled);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->http_status, 200);
  EXPECT_EQ(Sipi::observability::Metrics::instance().wedged_threads.Value(), before);
}
