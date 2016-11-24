/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */
/*!
 * SipiImage is the core object of dealing with images within the Sipi package
 * The SipiImage object holds all the information about an image and offers the methods
 * to read, write and modify images. Reading and writing is supported in several standard formats
 * such as TIFF, J2k, PNG etc.
 */
#ifndef __sipi_image_h
#define __sipi_image_h

#include <sstream>
#include <utility>
#include <string>
#include <map>

#include "SipiError.h"
#include "SipiIO.h"
#include "formats/SipiIOTiff.h"
#include "metadata/SipiXmp.h"
#include "metadata/SipiIcc.h"
#include "metadata/SipiIptc.h"
#include "metadata/SipiExif.h"
#include "metadata/SipiEssentials.h"
#include "iiifparser/SipiRegion.h"
#include "iiifparser/SipiSize.h"

#include "shttps/Connection.h"
#include "shttps/Hash.h"


/*!
 * \namespace Sipi Is used for all Sipi things.
 */
namespace Sipi {

    typedef unsigned char byte;
    typedef unsigned short word;

    /*! Implements the values of the photometric tag of the TIFF format */
    typedef enum : unsigned short {
        MINISWHITE = 0,     //!< B/W or gray value image with 0 = white and 1 (255) = black
        MINISBLACK = 1,     //!< B/W or gray value image with 0 = black and 1 (255) = white (is default in SIPI)
        RGB = 2,            //!< Color image with RGB values
        PALETTE = 3,        //!< Palette color image, is not suppoted by Sipi
        MASK = 4,           //!< Mask image, not supported by Sipi
        SEPARATED = 5,      //!< Color separated image, is assumed to be CMYK
        YCBCR = 6,          //!< Color representation with YCbCr, is supported by Sipi, but converted to an ordinary RGB
        CIELAB = 8,         //!< CIE*a*b image, only very limited support (untested!)
        ICCLAB = 9,         //!< ICCL*a*b image, only very limited support (untested!)
        ITULAB = 10,        //!< ITUL*a*b image, not supported yet (what is this by the way?)
        CFA = 32803,        //!< Color field array, used for DNG and RAW image. Not supported!
        LOGL = 32844,       //!< LOGL format (not supported)
        LOGLUV = 32845,     //!< LOGLuv format (not supported)
        LINEARRAW = 34892   //!< Linear raw array for DNG and RAW formats. Not supported!
    } PhotometricInterpretation;

    /*! The meaning of extra channels as used in the TIF format */
    typedef enum : unsigned short {
        UNSPECIFIED = 0,    //!< Unknown meaning
        ASSOCALPHA = 1,     //!< Associated alpha channel
        UNASSALPHA = 2      //!< Unassociated alpha channel
    } ExtraSamples;

    typedef enum {
        SKIP_NONE = 0x00,
        SKIP_ICC = 0x01,
        SKIP_XMP = 0x02,
        SKIP_IPTC = 0x04,
        SKIP_EXIF = 0x08,
        SKIP_ALL = 0xFF
    } SkipMetadata;

    /*!
    * This class implements the error handling for the different image formats.
    * It's being derived from the runtime_error so that catching the runtime error
    * also catched errors withing reading/writing an image format.
    */
    //class SipiImageError : public std::runtime_error {
    class SipiImageError  {
    private:
        std::string file; //!< Source file where the error occurs in
        int line; //!< Line within the source file
        int errnum; //!< error number if a system call is the reason for the error
        std::string errmsg;
    public:
        /*!
        * Constructor
        * \param[in] file_p The source file name (usually __FILE__)
        * \param[in] line_p The line number in the source file (usually __LINE__)
        * \param[in] Errnum, if a unix system call is the reason for throwing this exception
        */
        inline SipiImageError(const char *file_p, int line_p, int errnum_p = 0)
        : file(file_p), line(line_p), errnum(errnum_p) {}

        /*!
        * Constructor
        * \param[in] file_p The source file name (usually __FILE__)
        * \param[in] line_p The line number in the source file (usually __LINE__)
        * \param[in] msg Error message describing the problem
        * \param[in] errnum_p Errnum, if a unix system call is the reason for throwing this exception
        */
        inline SipiImageError(const char *file_p, int line_p, const char *msg_p, int errnum_p = 0)
        : file(file_p), line(line_p), errnum(errnum_p), errmsg(msg_p) {}

        /*!
        * Constructor
        * \param[in] file_p The source file name (usually __FILE__)
        * \param[in] line_p The line number in the source file (usually __LINE__)
        * \param[in] msg_p Error message describing the problem
        * \param[in] errnum_p Errnum, if a unix system call is the reason for throwing this exception
        */
        inline SipiImageError(const char *file_p, int line_p, const std::string &msg_p, int errnum_p = 0)
        : file(file_p), line(line_p), errnum(errnum_p), errmsg(msg_p) {}

       /*!
        * Returns the error as string
        */
        inline std::string get_error(void) const {
            stringstream ss;
            ss << "SIPI-IMAGE-ERROR at [" << file << ": #" << line << "]";
            if (errnum != 0) ss << ": " << strerror(errnum);
            ss << ": " << errmsg;
            return ss.str();
        }
        //----------------------------------------------------------------------

        inline friend std::ostream &operator<<(std::ostream &outstr, const SipiImageError &rhs)
        {
            outstr << endl << "SIPI-IMAGE-ERROR at [" << rhs.file << ": #" << rhs.line << "]" << endl;
            if (rhs.errnum != 0) outstr << "System error: " << strerror(rhs.errnum) << endl;
            outstr << "Description : " << rhs.errmsg << endl;
            return outstr;
        }
        //----------------------------------------------------------------------

        inline friend std::shared_ptr<Logger> &operator<<(std::shared_ptr<Logger> &log, const SipiImageError &rhs)
        {
            *log << Logger::LogLevel::ERROR << "SIPI-IMAGE-ERROR at [" << rhs.file << ": #" << rhs.line << "]: " << rhs.errmsg << Logger::LogAction::FLUSH;
            return log;
        }
        //----------------------------------------------------------------------

    };


    /*!
    * \class SipiImage
    *
    * Base class for all images in the Sipi package.
    * This class implements all the data and handling (methods) associated with
    * images in Sipi. Please note that the map of io-classes (see \ref SipiIO) has to
    * be instanciated in the SipiImage.cpp! Thus adding a new file format requires that SipiImage.cpp
    * is being modified!
    */
    class SipiImage {
        friend class SipiIcc;       //!< We need SipiIcc as friend class
        friend class SipiIOTiff;    //!< I/O class for the TIFF file format
        friend class SipiIOJ2k;     //!< I/O class for the JPEG2000 file format
        friend class SipiIOOpenJ2k; //!< I/O class for the JPEG2000 file format
        friend class SipiIOJpeg;    //!< I/O class for the JPEG file format
        friend class SipiIOPng;     //!< I/O class for the PNG file format
    private:
        static std::map<std::string,SipiIO*> io; //!< member variable holding a map of I/O class instances for the different file formats
        byte bilinn (byte buf[], register int nx, register float x, register float y, register int c, register int n);
        word bilinn (word buf[], register int nx, register float x, register float y, register int c, register int n);
    protected:
        int nx;         //!< Number of horizontal pixels (width)
        int ny;         //!< Number of vertical pixels (height)
        int nc;         //!< Total number of samples per pixel
        int bps;        //!< bits per sample. Currently only 8 and 16 are supported
        std::vector<ExtraSamples> es; //!< meaning of extra samples
        PhotometricInterpretation photo;    //!< Image type, that is the meaning of the channels
        byte *pixels;   //!< Pointer to block of memory holding the pixels
        SipiXmp *xmp;   //!< Pointer to instance SipiXmp class (\ref SipiXmp), or NULL
        SipiIcc *icc;   //!< Pointer to instance of SipiIcc class (\ref SipiIcc), or NULL
        SipiIptc *iptc; //!< Pointer to instance of SipiIptc class (\ref SipiIptc), or NULL
        SipiExif *exif; //!< Pointer to instance of SipiExif class (\ref SipiExif), or NULL
        SipiEssentials emdata; //!< Metadata to be stored in file header
        shttps::Connection *conobj; //!< Pointer to mongoose webserver connection data
        SkipMetadata skip_metadata; //!< If true, all metadata is stripped off
    public:
        static std::map<std::string, std::string> mimetypes; //! format (key) to mimetype (value) conversion map
        /*!
         * Default constructor. Creates an empty image
         */
        SipiImage();

        /*!
         * Copy constructor. Makes a deep copy of the image
         *
         * \param[in] img_p An existing instance if SipiImage
         */
        SipiImage(const SipiImage &img_p);

       /*!
        * Create an empty image with the pixel buffer available, but all pixels set to 0
        *
        * \param[in] nx_p Dimension in x direction
        * \param[in] ny_p Dimension in y direction
        * \param[in] nc_p Number of channels
        * \param[in] bps_p Bits per sample, either 8 or 16 are allowed
        * \param[in] photo_p The photometric interpretation
        */
        SipiImage(int nx_p, int ny_p, int nc_p, int bps_p, PhotometricInterpretation photo_p);

        /*!
         * Checks if the actual mimetype of an image file corresponds to the indicated mimetype and the extension of the filename.
         * This function is used to check if information submitted with a file are actually valid.
         */
        static bool checkMimeTypeConsistency(const std::string &path, const std::string &given_mimetype, const std::string &filename);

       /*!
        * Getter for nx
        */
        inline int getNx() { return nx; };


       /*!
        * Getter for ny
        */
        inline int getNy() { return ny; };

       /*!
        * Getter for nc (includes alpha channels!)
        */
        inline int getNc() { return nc; };

       /*!
        * Getter for number of alpha channels
        */
        inline int getNalpha() { return es.size(); }

        /*! Destructor
         *
         * Destroys the image and frees all the resources associated with it
         */
        ~SipiImage();

       /*!
        * Sets a pixel to a given value
        *
        * \param[in] x X position
        * \param[in] y Y position
        * \param[in] c Color channels
        * \param[in] val Pixel value
        */
        inline void setPixel(int x, int y, int c, int val) {
            if ((x < 0) || (x >= nx)) throw ((int) 1);
            if ((y < 0) || (x >= ny)) throw ((int) 2);
            if ((c < 0) || (x >= nc)) throw ((int) 3);
            switch (bps) {
                case 8: {
                    if (val > 0xff) throw ((int) 4);
                    register unsigned char *tmp = (unsigned char *) pixels;
                    tmp[nc*(x*nx + y) + c] = (unsigned char) val;
                    break;
                }
                case 16: {
                    if (val > 0xffff) throw ((int) 5);
                    register unsigned short *tmp = (unsigned short *) pixels;
                    tmp[nc*(x*nx + y) + c] = (unsigned short) val;
                    break;
                }
                default: {
                    if (val > 0xffff) throw ((int) 6);
                }
            }
        };

        /*!
         * Assignment operator
         *
         * Makes a deep copy of the instance
         *
         * \param[in] img_p Instance of a SipiImage
         */
        SipiImage& operator=(const SipiImage &img_p);

        /*!
         * Set the metadata that should be skipped in writing a file
         *
         * \param[in] smd Logical "or" of bitmasks for metadata to be skipped
         */
        inline void setSkipMetadata(SkipMetadata smd) { skip_metadata = smd; };


       /*!
        * Stores the connection parameters of the shttps server in an Image instance
        *
        * \param[in] conn_p Pointer to connection data
        */
        inline void connection(shttps::Connection *conobj_p) {conobj = conobj_p;};

       /*!
        * Retrieves the connection parameters of the mongoose server from an Image instance
        *
        * \returns Pointer to connection data
        */
         inline shttps::Connection *connection() { return conobj; };

         inline void essential_metadata(const SipiEssentials &emdata_p) { emdata = emdata_p; }

         inline SipiEssentials essential_metadata(void) { return emdata; }

       /*!
        * Read an image from the given path
        *
        * \param[in] filepath A string containing the path to the image file
        * \param[in] region Pointer to a SipiRegion which indicates that we
        *            are only interested in this regeion. The image will be cropped.
        * \param[in] size Pointer to a size object. The image will be scaled accordingly
        * \param[in] force_bps_8 We want in any case a 8 Bit/sample image. Reduce if necessary
        *
        * \throws SipiError
        */
        void read(std::string filepath, SipiRegion *region = NULL, SipiSize *size = NULL, bool force_bps_8 = false);

       /*!
        * Read an image that is to be considered an "original image". In this case
        * a SipiEssentials object is created containing the original name, the
        * original mime type. In addition also a checksum of the pixel values
        * is added in order to guarantee the integrity of the image pixels.
        * if the image is written as J2K or as TIFF image, these informations
        * are added to the file header (in case of TIFF as a private tag 65111,
        * in case of J2K as comment box).
        * If the file read already contains a SipiEssentials as embedded metadata,
        * it is not overwritten, put the embeded and pixel checksums are compared.
        *
        * \param[in] filepath A string containing the path to the image file
        * \param[in] region Pointer to a SipiRegion which indicates that we
        *            are only interested in this regeion. The image will be cropped.
        * \param[in] size Pointer to a size object. The image will be scaled accordingly
        * \param[in] htype The checksum method that should be used if the checksum is
        *            being calculated for the first time.
        *
        * \returns true, if everythin worked. False, if the checksums do not match.
        */
        bool readOriginal(const std::string &filepath, SipiRegion *region, SipiSize *size, shttps::HashType htype = shttps::HashType::sha256);

        /*!
         * Read an image that is to be considered an "original image". In this case
         * a SipiEssentials object is created containing the original name, the
         * original mime type. In addition also a checksum of the pixel values
         * is added in order to guarantee the integrity of the image pixels.
         * if the image is written as J2K or as TIFF image, these informations
         * are added to the file header (in case of TIFF as a private tag 65111,
         * in case of J2K as comment box).
         * If the file read already contains a SipiEssentials as embedded metadata,
         * it is not overwritten, put the embeded and pixel checksums are compared.
         *
         * \param[in] filepath A string containing the path to the image file
         * \param[in] region Pointer to a SipiRegion which indicates that we
         *            are only interested in this regeion. The image will be cropped.
         * \param[in] size Pointer to a size object. The image will be scaled accordingly
         * \param[in] origname Original file name
         * \param[in] htype The checksum method that should be used if the checksum is
         *            being calculated for the first time.
         *
         * \returns true, if everythin worked. False, if the checksums do not match.
         */
        bool readOriginal(const std::string &filepath, SipiRegion *region, SipiSize *size, const std::string &origname, shttps::HashType htype);


       /*!
        * Get the dimension of the image
        *
        * \param[in] filepath Pathname of the image file
        * \param[out] width Width of the image in pixels
        * \param[out] height Height of the image in pixels
        */
        static void getDim(std::string filepath, int &width, int &height);

        void getDim(int &width, int &height);

       /*!
        * Write an image to somewhere
        *
        * This method writes the image to a destination. The destination can be
        * - a file if w path (filename) is given
        * - stdout of the filepath is "-"
        * - to the websocket, if the filepath is the string "HTTP" (given the webserver is activated)
        *
        * \param[in] ftype The file format that should be used to write the file. Supported are
        * - "tif" for TIFF files
        * - "j2k" for JPEG2000 files
        * - "png" for PNG files
        * \param[in] filepath String containg the path/filename
        */
        void write(std::string ftype, std::string filepath, int quality = -1);

        /*!
         * Converts the image representation
         *
         * \param[in] target_icc_p ICC profile which determines the new image representation
         * \param[in] bps Bits/sample of the new image representation
         */
        void convertToIcc(const SipiIcc &target_icc_p, int bps);


        /*!
         * Removes a channel from a multi component image
         *
         * \param[in] chan Index of component to remove, starting with 0
         */
        void removeChan(unsigned int chan);

       /*!
        * Crops an image to a region
        *
        * \param[in] nx Horizontal start position of region. If negative, it's set to 0, and the width is adjusted
        * \param[in] ny Vertical start position of region. If negative, it's set to 0, and the height is adjusted
        * \param[in] width Width of the region. If the region goes beyond the image dimensions, it's adjusted.
        * \param[in] height Height of the region. If the region goes beyond the image dimensions, it's adjusted
        */
        bool crop(int x, int y, int width = 0, int height = 0);

       /*!
        * Crops an image to a region
        *
        * \param[in] Pointer to SipiRegion
        * \param[in] ny Vertical start position of region. If negative, it's set to 0, and the height is adjusted
        * \param[in] width Width of the region. If the region goes beyond the image dimensions, it's adjusted.
        * \param[in] height Height of the region. If the region goes beyond the image dimensions, it's adjusted
        */
        bool crop(SipiRegion *region);

       /*!
        * Resize an image
        *
        * \param[in] nnx New horizonal dimension (width)
        * \param[in] nny New vertical dimension (height)
        */
        bool scale(int nnx = 0, int nny = 0);


       /*!
        * Rotate an image
        *
        * The angles 0, 90, 180, 270 are treated specially!
        *
        * \param[in] angle Rotation angle
        * \param[in] mirror If true, mirror the image before rotation
        */
        bool rotate(float angle, bool mirror = false);

       /*!
        * Convert an image from 16 to 8 bit. The algorithm just divides all pixel values
        * by 256 using the ">> 8" operator (fast & efficient)
        *
        * \returns Returns true on success, false on error
        */
        bool to8bps(void);

       /*!
        * Add a watermark to a file...
        *
        * \param[in] wmfilename Path to watermakfile (which must be a TIFF file at the moment)
        */
        bool add_watermark(std::string wmfilename);


       /*!
        * Calcaulates the difference between 2 images.
        *
        * The differfence between 2 images can conatin (and usually will) negative values.
        * In order to create a standard image, the values at "0" will be liftet to 127 (8-Bit images)
        * or 32767. The span will be defined by max(minimum, maximum), where minimum and maximum are
        * absolute values. Thus a new pixelvalue will be calculated as follows:
        * ```
        * int maxmax = abs(min) > abs(max) ? abs(min) : abs(min);
        * newval = (byte) ((oldval + maxmax)*UCHAR_MAX/(2*maxmax));
        * ```
        * \param[in] rhs right hand side of "-="
        */
        SipiImage &operator-=(const SipiImage &rhs);

       /*!
        * Calcaulates the difference between 2 images.
        *
        * The differfence between 2 images can conatin (and usually will) negative values.
        * In order to create a standard image, the values at "0" will be liftet to 127 (8-Bit images)
        * or 32767. The span will be defined by max(minimum, maximum), where minimum and maximum are
        * absolute values. Thus a new pixelvalue will be calculated as follows:
        * ```
        * int maxmax = abs(min) > abs(max) ? abs(min) : abs(min);
        * newval = (byte) ((oldval + maxmax)*UCHAR_MAX/(2*maxmax));
        * ```
        *
        * \param[in] lhs left-hand side of "-" operator
        * \param[in] rhs right hand side of "-" operator
        */
        SipiImage &operator-(const SipiImage &rhs);

        SipiImage &operator+=(const SipiImage &rhs);

        SipiImage &operator+(const SipiImage &rhs);

        bool operator== (const SipiImage &rhs);

        /*!
        * The overloaded << operator which is used to write the error message to the output
        *
        * \param[in] lhs The output stream
        * \param[in] rhs Reference to an instance of a SipiImage
        * \returns Returns ostream object
        */
        friend std::ostream &operator<< (std::ostream &lhs, const SipiImage &rhs);
    };
}

#endif
