/*
 * Copyright © 2016 - 2024 Swiss National Data and Service Center for the Humanities and/or DaSCH Service Platform
 * contributors. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <mutex>
#include <ostream>
#include <utility>
#include <vector>

#include "metadata/xmp.h"

/*!
 * SIPI stores the XMP packet verbatim: it neither parses nor validates the
 * RDF/XML payload, because Exiv2's XMP parser is not thread-safe at the
 * Exiv2 version SIPI depends on. xmpBytes() returns exactly the bytes it
 * was constructed with.
 */

namespace Sipi {

XmpMutex xmp_mutex;

void xmplock_func(void *pLockData, bool lockUnlock)
{
  auto *m = static_cast<XmpMutex *>(pLockData);
  if (lockUnlock) {
    m->lock.lock();
  } else {
    m->lock.unlock();
  }
}
//=========================================================================

Xmp::Xmp(const std::string &xmp) { __xmpstr = xmp; }
//============================================================================

Xmp::Xmp(const char *xmp) { __xmpstr = xmp; }
//============================================================================

Xmp::Xmp(const char *xmp, int len)
{
  std::string buf(xmp, len);
  __xmpstr = buf;
}
//============================================================================


Xmp::~Xmp() {}
//============================================================================


// Returns the RDF/XML packet exactly as it was constructed.
std::string Xmp::xmpBytes() { return __xmpstr; }
//============================================================================

namespace {

  bool is_name_char(char c)
  {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-' || c == '.' || c == ':';
  }

  std::size_t skip_space(const std::string &s, std::size_t i)
  {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) { ++i; }
    return i;
  }

  // A real packet binds a namespace a few times at most; the cap keeps a
  // hostile one from multiplying the passes.
  constexpr std::size_t kMaxBindings = 8;
  constexpr std::size_t kMaxStartTagBytes = 256;
  // An element to erase may declare namespaces in its start tag.
  constexpr std::size_t kMaxErasedStartTagBytes = 4096;

  // The prefixes the packet binds to `uri` (`xmlns:p="uri"`, at most
  // kMaxBindings), plus the conventional `defaults`, which always apply.
  std::vector<std::string>
    prefixes_bound_to(const std::string &s, const std::string &uri, std::initializer_list<const char *> defaults)
  {
    std::vector<std::string> prefixes;
    for (std::size_t pos = s.find("xmlns:"); pos != std::string::npos && prefixes.size() < kMaxBindings;
         pos = s.find("xmlns:", pos + 1)) {
      std::size_t i = pos + 6;
      const std::size_t name_start = i;
      while (i < s.size() && is_name_char(s[i]) && s[i] != ':') { ++i; }
      const std::string prefix = s.substr(name_start, i - name_start);
      i = skip_space(s, i);
      if (prefix.empty() || i >= s.size() || s[i] != '=') { continue; }
      i = skip_space(s, i + 1);
      if (i >= s.size() || (s[i] != '"' && s[i] != '\'')) { continue; }
      if (s.compare(i + 1, uri.size(), uri) == 0 && i + 1 + uri.size() < s.size() && s[i + 1 + uri.size()] == s[i]
          && std::find(prefixes.begin(), prefixes.end(), prefix) == prefixes.end()) {
        prefixes.push_back(prefix);
      }
    }
    for (const char *d : defaults) {
      if (std::find(prefixes.begin(), prefixes.end(), d) == prefixes.end()) { prefixes.emplace_back(d); }
    }
    return prefixes;
  }

  // Replaces the integer value of every `name="123"` attribute and
  // `<name ...>123</name>` element. One forward pass; the search for the end
  // of a start tag is bounded, so unterminated tags cannot make it quadratic.
  void set_integer_property(std::string &s, const std::string &name, const std::string &value)
  {
    std::string out;
    std::size_t copied = 0;
    for (std::size_t pos = s.find(name); pos != std::string::npos; pos = s.find(name, pos + 1)) {
      const std::size_t after = pos + name.size();
      if (after < s.size() && is_name_char(s[after])) { continue; }
      const char before = pos == 0 ? ' ' : s[pos - 1];
      std::size_t digits = std::string::npos;
      char terminator = 0;
      if (before == '<') {
        const auto limit = s.begin() + static_cast<std::ptrdiff_t>(std::min(s.size(), after + kMaxStartTagBytes));
        const auto close = std::find(s.begin() + static_cast<std::ptrdiff_t>(after), limit, '>');
        if (close == limit) { continue; }
        digits = skip_space(s, static_cast<std::size_t>(close - s.begin()) + 1);
      } else if (!is_name_char(before) && before != '/') {
        std::size_t i = skip_space(s, after);
        if (i >= s.size() || s[i] != '=') { continue; }
        i = skip_space(s, i + 1);
        if (i >= s.size() || (s[i] != '"' && s[i] != '\'')) { continue; }
        terminator = s[i];
        digits = i + 1;
      } else {
        continue;
      }
      std::size_t end = digits;
      while (end < s.size() && std::isdigit(static_cast<unsigned char>(s[end])) != 0) { ++end; }
      if (end == digits) { continue; }
      if (terminator != 0) {
        if (end >= s.size() || s[end] != terminator) { continue; }
      } else if (s.compare(skip_space(s, end), name.size() + 3, "</" + name + ">") != 0) {
        continue;
      }
      out.append(s, copied, digits - copied);
      out += value;
      copied = end;
    }
    if (copied > 0) {
      out.append(s, copied);
      s = std::move(out);
    }
  }

  // Removes every `<name ...>...</name>` and `<name .../>` element. False if
  // a start tag or an element has no end.
  bool erase_element(std::string &s, const std::string &name)
  {
    const std::string open = "<" + name;
    const std::string close = "</" + name;
    std::string out;
    std::size_t copied = 0;
    std::size_t pos = s.find(open);
    while (pos != std::string::npos) {
      const std::size_t after = pos + open.size();
      if (after < s.size() && is_name_char(s[after])) {
        pos = s.find(open, after);
        continue;
      }
      const auto limit = s.begin() + static_cast<std::ptrdiff_t>(std::min(s.size(), after + kMaxErasedStartTagBytes));
      const auto gt = std::find(s.begin() + static_cast<std::ptrdiff_t>(after), limit, '>');
      if (gt == limit) { return false; }
      std::size_t end = static_cast<std::size_t>(gt - s.begin()) + 1;
      if (*(gt - 1) != '/') {
        std::size_t c = s.find(close, end);
        while (c != std::string::npos && c + close.size() < s.size() && is_name_char(s[c + close.size()])) {
          c = s.find(close, c + close.size());
        }
        if (c == std::string::npos) { return false; }
        end = skip_space(s, c + close.size());
        if (end >= s.size() || s[end] != '>') { return false; }
        ++end;
      }
      out.append(s, copied, pos - copied);
      copied = end;
      pos = s.find(open, end);
    }
    if (copied > 0) {
      out.append(s, copied);
      s = std::move(out);
    }
    return true;
  }

}// namespace

void Xmp::set_pixel_dimensions(std::size_t w, std::size_t h)
{
  // Any prefix bound to a namespace names the same properties.
  const std::string wv = std::to_string(w);
  const std::string hv = std::to_string(h);
  for (const auto &p : prefixes_bound_to(__xmpstr, "http://ns.adobe.com/exif/1.0/", { "exif" })) {
    set_integer_property(__xmpstr, p + ":PixelXDimension", wv);
    set_integer_property(__xmpstr, p + ":PixelYDimension", hv);
  }
  for (const auto &p : prefixes_bound_to(__xmpstr, "http://ns.adobe.com/tiff/1.0/", { "tiff" })) {
    set_integer_property(__xmpstr, p + ":ImageWidth", wv);
    set_integer_property(__xmpstr, p + ":ImageLength", hv);
  }
  for (const auto &p :
    prefixes_bound_to(__xmpstr, "http://iptc.org/std/Iptc4xmpExt/2008-02-29/", { "Iptc4xmpExt" })) {
    set_integer_property(__xmpstr, p + ":MaxAvailWidth", wv);
    set_integer_property(__xmpstr, p + ":MaxAvailHeight", hv);
  }
}
//============================================================================

bool Xmp::erase_thumbnails()
{
  for (const auto &p : prefixes_bound_to(__xmpstr, "http://ns.adobe.com/xap/1.0/", { "xmp", "xap" })) {
    if (!erase_element(__xmpstr, p + ":Thumbnails")) { return false; }
  }
  for (const auto &p : prefixes_bound_to(__xmpstr, "http://ns.adobe.com/xap/1.0/g/img/", { "xmpGImg", "xapGImg" })) {
    if (!erase_element(__xmpstr, p + ":image")) { return false; }
  }
  // A text scan cannot resolve every binding (a default namespace, a prefix
  // past the cap): a Thumbnails element or GImg image left over fails.
  for (std::size_t pos = __xmpstr.find("Thumbnails"); pos != std::string::npos;
       pos = __xmpstr.find("Thumbnails", pos + 1)) {
    const char before = pos == 0 ? ' ' : __xmpstr[pos - 1];
    const std::size_t after = pos + 10;
    if ((before == ':' || before == '<') && (after >= __xmpstr.size() || !is_name_char(__xmpstr[after]))) {
      return false;
    }
  }
  return __xmpstr.find("GImg:image") == std::string::npos;
}
//============================================================================

std::ostream &operator<<(std::ostream &outstr, const Xmp &) { return outstr; }
//============================================================================

}// namespace Sipi
