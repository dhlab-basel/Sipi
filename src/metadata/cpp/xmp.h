/*
 * Copyright © 2016 - 2024 Swiss National Data and Service Center for the Humanities and/or DaSCH Service Platform
 * contributors. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef SIPI_METADATA_XMP_H
#define SIPI_METADATA_XMP_H

#include <cstddef>
#include <iosfwd>
#include <mutex>
#include <string>

namespace Sipi {

typedef struct
{
  std::mutex lock;
} XmpMutex;

extern XmpMutex xmp_mutex;

extern void xmplock_func(void *pLockData, bool lockUnlock);

/*!
 * This class holds an XMP metadata packet. SIPI stores the RDF/XML payload
 * verbatim, neither parsing nor validating it, because Exiv2's XMP parser
 * is not thread-safe at the Exiv2 version SIPI depends on. The only edits are
 * set_pixel_dimensions() and erase_thumbnails(), text substitutions a
 * size-restricted output applies.
 */
class Xmp
{
private:
  std::string __xmpstr;

public:
  /*!
   * Constructor
   *
   * \param[in] xmp A std::string containing RDF/XML with XMP data
   */
  Xmp(const std::string &xmp);

  /*!
   * Constructor
   *
   * \param[in] xmp A C-string (char *)containing RDF/XML with XMP data
   */
  Xmp(const char *xmp);

  /*!
   * Constructor
   *
   * \param[in] xmp A string containing RDF/XML with XMP data
   * \param[in] len Length of the string
   */
  Xmp(const char *xmp, int len);


  /*!
   * Destructor
   */
  ~Xmp();


  /*!
   * Returns the bytes of the RDF/XML data as a std::string.
   *
   * @return String holding the xmp data
   */
  std::string xmpBytes();

  /*!
   * Sets exif:PixelXDimension/PixelYDimension, tiff:ImageWidth/ImageLength and
   * Iptc4xmpExt:MaxAvailWidth/MaxAvailHeight, in attribute or element form and
   * under any prefix the packet binds to their namespace, to w x h where the
   * value is an integer. Every other byte of the packet is left as it is.
   */
  void set_pixel_dimensions(std::size_t w, std::size_t h);

  /*!
   * Removes every xmp:Thumbnails property (the embedded preview images, with
   * their xmpGImg size fields) and every other xmpGImg:image, under any prefix
   * the packet binds to their namespace. False if an element has no end or a
   * Thumbnails element or GImg image is left that the scan cannot attribute.
   */
  [[nodiscard]] bool erase_thumbnails();
};

/*!
 * Lets an Xmp appear in SipiImage's stream output. It emits nothing: SIPI
 * holds the XMP packet as an opaque RDF/XML byte string with no parsed
 * structure to format.
 *
 * \param[in] lhs The output stream
 * \param[in] rhs Reference to an instance of a Xmp
 * \returns Returns ostream object
 */
std::ostream &operator<<(std::ostream &lhs, const Xmp &rhs);

}// namespace Sipi

#endif
