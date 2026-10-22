/*
 * Copyright © 2016 - 2024 Swiss National Data and Service Center for the Humanities and/or DaSCH Service Platform
 * contributors. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include <exiv2/photoshop.hpp>

#include "error/SipiError.h"
#include "metadata/exif.h"
#include "metadata/xmp.h"

namespace Sipi {

Exif::Exif()
{
  byteorder = Exiv2::littleEndian;// that's today's default....
};
//============================================================================


Exif::Exif(Exiv2::ExifData data, Exiv2::ByteOrder order, std::vector<unsigned char> binary)
  : binaryExif(std::move(binary)), exifData(std::move(data)), byteorder(order)
{}
//============================================================================

Result<std::shared_ptr<Exif>> Exif::parse(const unsigned char *exif, unsigned int len)
{
  Exiv2::ExifData data;
  Exiv2::ByteOrder order;
  try {
    order = Exiv2::ExifParser::decode(data, exif, (uint32_t)len);
  } catch (Exiv2::Error &exiverr) {
    return std::unexpected(SipiValueError{ ErrorCode::kMetadataParseFailed, exiverr.what() });
  }

  //
  // we save the binary exif... we use it later for constructing a binary exif again!
  //
  std::vector<unsigned char> binary(exif, exif + len);
  return std::shared_ptr<Exif>(new Exif(std::move(data), order, std::move(binary)));
}
//============================================================================

// Type-dispatched assignment helpers backing the inline `getValByKey<T>`
// templates in the header. Moved out-of-line per DEV-6407 to keep
// `metadata/exif.h` lean.

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::string &val)
{
  val = v->toString();
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<std::string> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(v->toString(i)); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, char &val)
{
  val = static_cast<char>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<char> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<char>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, unsigned char &val)
{
  val = static_cast<unsigned char>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<unsigned char> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<unsigned char>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, short &val)
{
  val = static_cast<short>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<short> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<short>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, unsigned short &val)
{
  val = static_cast<unsigned short>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<unsigned short> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<unsigned short>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, int &val)
{
  val = static_cast<int>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<int> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<int>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, unsigned int &val)
{
  val = static_cast<unsigned int>(v->toInt64());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<unsigned int> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<unsigned int>(v->toInt64(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, float &val)
{
  val = static_cast<float>(v->toFloat());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<float> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<float>(v->toFloat(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, double &val)
{
  val = static_cast<double>(v->toFloat());
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<double> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(static_cast<double>(v->toFloat(i))); }
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, Exiv2::Rational &val)
{
  val = v->toRational();
  return v->ok();
}

bool Exif::assign_val(Exiv2::Value::UniquePtr &v, std::vector<Exiv2::Rational> &val)
{
  for (int i = 0; i < v->count(); i++) { val.push_back(v->toRational(i)); }
  return v->ok();
}

std::vector<unsigned char> Exif::exifBytes()
{
  Exiv2::Blob blob;
  Exiv2::WriteMethod wm =
    Exiv2::ExifParser::encode(blob, binaryExif.data(), static_cast<uint32_t>(binaryExif.size()), byteorder, exifData);
  if (wm == Exiv2::wmIntrusive) {
    // Refresh the cached binary blob so subsequent reads see the encoded form.
    binaryExif.assign(blob.begin(), blob.end());
  }
  return binaryExif;
}
//============================================================================

namespace {

  // A superset of the previews exiv2's PreviewManager reads (src/preview.cpp,
  // 0.28): re-check on an exiv2 bump. Maker-note tags are matched by name in
  // every maker-note group.
  bool is_preview(const Exiv2::Exifdatum &datum)
  {
    static const std::set<std::string> groups = { "Thumbnail",
      "SubImage1",
      "SubImage2",
      "SubImage3",
      "SubImage4",
      "SubImage5",
      "SubImage6",
      "SubImage7",
      "SubImage8",
      "SubImage9",
      "SubThumb1",
      "Image2",
      "Image3",
      "NikonPreview",
      "SamsungPreview" };
    static const std::set<std::string> keys = { "Exif.Image.JPEGInterchangeFormat",
      "Exif.Image.JPEGInterchangeFormatLength",
      "Exif.PanasonicRaw.PreviewImage" };
    static const std::set<std::string> maker_note_tags = { "Preview",
      "PreviewImage",
      "PreviewImageStart",
      "PreviewImageLength",
      "PreviewImageValid",
      "PreviewOffset",
      "PreviewLength",
      "Thumbnail",
      "ThumbnailImage",
      "ThumbnailOffset",
      "ThumbnailLength" };
    return groups.contains(datum.groupName()) || keys.contains(datum.key())
           || (Exiv2::ExifTags::isMakerGroup(datum.groupName()) && maker_note_tags.contains(datum.tagName()));
  }

  // `values` as the datum's type reads them: a rational as n/1.
  std::string as_value(const Exiv2::Exifdatum &datum, std::initializer_list<std::size_t> values)
  {
    const bool rational = datum.typeId() == Exiv2::unsignedRational || datum.typeId() == Exiv2::signedRational;
    std::string s;
    for (const std::size_t v : values) {
      if (!s.empty()) { s += ' '; }
      s += std::to_string(v);
      if (rational) { s += "/1"; }
    }
    return s;
  }

}// namespace

bool Exif::set_pixel_dimensions(std::size_t w, std::size_t h)
{
  bool ok = true;
  for (auto &datum : exifData) {
    const std::string tag = datum.tagName();
    std::string value;
    if (tag == "PixelXDimension" || tag == "ImageWidth" || tag == "RelatedImageWidth") {
      value = as_value(datum, { w });
    } else if (tag == "PixelYDimension" || tag == "ImageLength" || tag == "ImageHeight"
               || tag == "RelatedImageLength") {
      value = as_value(datum, { h });
    } else if (tag == "DefaultCropSize" || tag == "OriginalDefaultFinalSize" || tag == "OriginalBestQualityFinalSize"
               || tag == "OriginalDefaultCropSize") {
      value = as_value(datum, { w, h });
    } else if (tag == "ActiveArea") {
      value = as_value(datum, { 0, 0, h, w });
    } else {
      continue;
    }
    ok = datum.setValue(value) == 0 && ok;
  }
  return ok;
}
//============================================================================

void Exif::erase_previews()
{
  bool maker_note = false;
  for (auto it = exifData.begin(); it != exifData.end();) {
    if (is_preview(*it)) {
      maker_note = maker_note || Exiv2::ExifTags::isMakerGroup(it->groupName());
      it = exifData.erase(it);
    } else {
      ++it;
    }
  }
  // Exiv2 rebuilds a decoded maker note from its remaining tags; the raw copy
  // would still carry the preview.
  if (maker_note) {
    if (auto pos = exifData.findKey(Exiv2::ExifKey("Exif.Photo.MakerNote")); pos != exifData.end()) {
      exifData.erase(pos);
    }
  }
}
//============================================================================

bool Exif::restrict_image_resources(std::size_t w, std::size_t h)
{
  const Exiv2::ExifKey key("Exif.Image.ImageResources");
  const auto pos = exifData.findKey(key);
  if (pos == exifData.end()) { return true; }
  // Only one block is rewritten; a second would pass as it is.
  if (std::count_if(exifData.begin(), exifData.end(), [&](const Exiv2::Exifdatum &d) { return d.key() == key.key(); })
      > 1) {
    return false;
  }
  std::vector<unsigned char> in(pos->size());
  pos->copy(in.data(), byteorder);
  std::vector<unsigned char> out;
  bool changed = false;
  std::size_t at = 0;
  while (at < in.size()) {
    if (std::all_of(in.begin() + static_cast<std::ptrdiff_t>(at), in.end(), [](unsigned char b) { return b == 0; })) {
      out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(at), in.end());
      break;
    }
    // Signature, 2-byte id, Pascal name padded to even, 4-byte size, data padded to even.
    const std::size_t left = in.size() - at;
    if (left < 12 || !Exiv2::Photoshop::isIrb(&in[at])) { return false; }
    const std::size_t head = 6 + ((in[at + 6] + 2U) & ~std::size_t{ 1 }) + 4;
    if (left < head) { return false; }
    const std::size_t len = (std::size_t{ in[at + head - 4] } << 24) | (std::size_t{ in[at + head - 3] } << 16)
                            | (std::size_t{ in[at + head - 2] } << 8) | in[at + head - 1];
    if (left - head < len) { return false; }
    const std::size_t end = at + std::min(left, head + len + len % 2);
    const auto data = in.begin() + static_cast<std::ptrdiff_t>(at + head);
    std::optional<std::vector<unsigned char>> replacement;
    switch ((in[at + 4] << 8) | in[at + 5]) {
    case 0x0409:// thumbnails
    case 0x040c:
      changed = true;
      at = end;
      continue;
    case 0x03e8:// legacy header: rows and columns
    case 0x041a:// slices: the bounds of the whole image
      return false;
    case 0x0424: {
      Xmp xmp(std::string(data, data + static_cast<std::ptrdiff_t>(len)));
      if (!xmp.erase_thumbnails()) { return false; }
      xmp.set_pixel_dimensions(w, h);
      const std::string bytes = xmp.xmpBytes();
      if (!std::equal(bytes.begin(), bytes.end(), data, data + static_cast<std::ptrdiff_t>(len))) {
        replacement.emplace(bytes.begin(), bytes.end());
      }
      break;
    }
    case 0x0422:// EXIF
    case 0x0423: {
      auto nested = parse(in.data() + at + head, static_cast<unsigned int>(len));
      if (!nested || (*nested)->byteorder == Exiv2::invalidByteOrder) { return false; }
      Exif &e = **nested;
      if (e.exifData.findKey(Exiv2::ExifKey("Exif.Image.ImageResources")) != e.exifData.end()) { return false; }
      e.erase_previews();
      if (!e.set_pixel_dimensions(w, h)) { return false; }
      std::vector<unsigned char> bytes = e.exifBytes();
      if (!std::equal(bytes.begin(), bytes.end(), data, data + static_cast<std::ptrdiff_t>(len))) {
        replacement = std::move(bytes);
      }
      break;
    }
    default:
      break;
    }
    if (replacement) {
      const std::size_t n = replacement->size();
      out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(at), data - 4);
      out.insert(out.end(),
        { static_cast<unsigned char>(n >> 24),
          static_cast<unsigned char>(n >> 16),
          static_cast<unsigned char>(n >> 8),
          static_cast<unsigned char>(n) });
      out.insert(out.end(), replacement->begin(), replacement->end());
      if (n % 2 == 1) { out.push_back(0); }
      changed = true;
    } else {
      out.insert(
        out.end(), in.begin() + static_cast<std::ptrdiff_t>(at), in.begin() + static_cast<std::ptrdiff_t>(end));
    }
    at = end;
  }
  if (!changed) { return true; }
  const Exiv2::TypeId type = pos->typeId();
  const std::size_t unit = std::max<std::size_t>(1, Exiv2::TypeInfo::typeSize(type));
  out.resize(out.size() + (unit - out.size() % unit) % unit, 0);
  auto value = Exiv2::Value::create(type);
  value->read(out.data(), out.size(), byteorder);
  pos->setValue(value.get());
  return true;
}
//============================================================================

Exiv2::Rational Exif::toRational(float f)
{
  if (!std::isfinite(f)) { return std::make_pair(0, 1); }

  const double value = static_cast<double>(f);
  if (value == 0.0) { return std::make_pair(0, 1); }
  if (std::trunc(value) == value && value >= static_cast<double>(INT_MIN) && value <= static_cast<double>(INT_MAX)) {
    return std::make_pair(static_cast<int>(value), 1);
  }

  int denominator = INT_MAX;
  double scaled = value * static_cast<double>(denominator);
  while ((scaled > static_cast<double>(INT_MAX) || scaled < static_cast<double>(INT_MIN)) && denominator > 1) {
    denominator /= 10;
    scaled = value * static_cast<double>(denominator);
  }

  if (denominator < 1) { denominator = 1; }

  if (scaled > static_cast<double>(INT_MAX)) { scaled = static_cast<double>(INT_MAX); }
  if (scaled < static_cast<double>(INT_MIN)) { scaled = static_cast<double>(INT_MIN); }

  int numerator = static_cast<int>(std::round(scaled));
  if (numerator == 0) { numerator = (value > 0.0) ? 1 : -1; }

  return std::make_pair(numerator, denominator);
}
//============================================================================

Exiv2::URational Exif::toURational(float f)
{
  if (!std::isfinite(f)) throw SipiError("Cannot convert non-finite float to URational!");
  if (f < 0.0F) throw SipiError("Cannot convert negative float to URational!");

  const double value = static_cast<double>(f);
  if (value == 0.0) { return std::make_pair(0U, 1U); }
  if (std::trunc(value) == value && value <= static_cast<double>(UINT_MAX)) {
    return std::make_pair(static_cast<unsigned int>(value), 1U);
  }

  unsigned int denominator = UINT_MAX;
  double scaled = value * static_cast<double>(denominator);
  while (scaled > static_cast<double>(UINT_MAX) && denominator > 1U) {
    denominator /= 10U;
    scaled = value * static_cast<double>(denominator);
  }

  if (denominator < 1U) { denominator = 1U; }
  if (scaled > static_cast<double>(UINT_MAX)) { scaled = static_cast<double>(UINT_MAX); }

  auto numerator = static_cast<unsigned int>(std::round(scaled));
  if (numerator == 0U) { numerator = 1U; }

  return std::make_pair(numerator, denominator);
}
//============================================================================


std::ostream &operator<<(std::ostream &outstr, Exif &rhs)
{
  auto end = rhs.exifData.end();
  for (auto i = rhs.exifData.begin(); i != end; ++i) {
    const char *tn = i->typeName();
    outstr << std::setw(44) << std::setfill(' ') << std::left << i->key() << " "
           << "0x" << std::setw(4) << std::setfill('0') << std::right << std::hex << i->tag() << " " << std::setw(9)
           << std::setfill(' ') << std::left << (tn ? tn : "Unknown") << " " << std::dec << std::setw(3)
           << std::setfill(' ') << std::right << i->count() << "  " << std::dec << i->value() << "\n";
  }
  return outstr;
}
//============================================================================

}// namespace Sipi
