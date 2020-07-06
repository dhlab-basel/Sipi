# Simple Image Presentation Interface (SIPI) - Introduction

SIPI has a threefold usage:

- it can act as a standard [IIIF](https://iiif.io) server
    - implementing the IIIF Image API V3 level2
    - implementing the IIIF Authentication API
    - implementing an extension to provaide access to multiple page PDF's using the IIIF URL syntax
- implementing a simple web server
    - serving HTML and other files
    - providing embeded [LUA](https://www.lua.org) scripting
    - providing embeded sqlite3 with LUA interface
    - configurable routing for RESTful services programmed in LUA
- acting as a stand-alone image conversion tool
    - from/to TIFF, JPEG200, PNG, JPEG, (PDF)
    - preserving embedded metadata (EXIF, IPTC, TIFF, XMP)
    - preserving and/or converting ICC color profiles
    
## SIPI as IIIF-Server
### Extensions to the IIIF-Standard

#### Access to PDF Pages
SIPI is able to deliver PDF's using an extended IIIF-URL to access a specific page of a multipage PDF.
- info.json includes a field `numpages` that indicates the total number of pages
  the PDF has
- the image-ID given in the IIIF URL must incude a pagenumber specificer
  `@pagenum` with an integer between 1 and the maximum number of pages, e.g.
  ```
  https://iiif.dummy.org/images/test.pdf@12/full/,1000/default.jpg
  ```
  The given URL would return page #12 of the PDF `test.pdf` with a hight of 1000 pixels.
  Thus, all IIIF URL parts will work as expected.


