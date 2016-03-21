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
#include <iostream>
#include <string>
#include <cmath>
#include "lcms2.h"

#include "SipiImage.h"
#include "formats/SipiIOTiff.h"
#include "formats/SipiIOJ2k.h"
//#include "formats/SipiIOOpenJ2k.h"
#include "formats/SipiIOJpeg.h"
#include "formats/SipiIOPng.h"
#include "USWebCoatedSWOP_icc.h"
#include "shttps/GetMimetype.h"

using namespace std;

static const char __file__[] = __FILE__;


namespace Sipi {

  std::map<std::string,SipiIO*> SipiImage::io = {
    {"tif", new SipiIOTiff()},
    {"jpx", new SipiIOJ2k()},
    //{"jpx", new SipiIOOpenJ2k()},
    {"jpg", new SipiIOJpeg()},
    {"png", new SipiIOPng()}
  };

  std::map<std::string, std::string> SipiImage::mimetypes = {
    {"jpx", "image/jp2"},
    {"jp2", "image/jp2"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"tiff", "image/tiff"},
    {"tif", "image/tiff"},
    {"png", "image/png"}
  };

    SipiImage::SipiImage() {
        nx = 0;
        ny = 0;
        nc = 0;
        ne = 0;
        bps = 0;
        es = NULL;
        pixels = NULL;
        xmp = NULL;
        icc = NULL;
        iptc = NULL;
        exif = NULL;
        skip_metadata = SKIP_NONE;
        conobj = NULL;
    };
    //============================================================================

    SipiImage::SipiImage(const SipiImage &img_p) {
        nx = img_p.nx;
        ny = img_p.ny;
        nc = img_p.nc;
        bps = img_p.bps;

        size_t bufsiz;
        switch (bps) {
            case 8: {
                bufsiz = nx*ny*nc*sizeof (unsigned char);
                break;
            }
            case 16: {
                bufsiz = nx*ny*nc*sizeof (unsigned short);
                break;
            }
            default: {
                bufsiz = 0;
            }
        }
        if (bufsiz > 0) {
            pixels = new byte[bufsiz];
            memcpy (pixels, img_p.pixels, bufsiz);
        }
        xmp = img_p.xmp; // Problem!!!! is the operator= overloaded to make a deep copy?
        icc = img_p.icc;
        iptc = img_p.iptc;
        exif = img_p.exif;
        skip_metadata = img_p.skip_metadata;
        conobj = img_p.conobj;
    }
    //============================================================================


    SipiImage::~SipiImage() {
        delete [] pixels;
        delete [] es;
        delete xmp;
        delete icc;
        delete iptc;
        delete exif;
    };
    //============================================================================


    SipiImage& SipiImage::operator=(const SipiImage &img_p) {
        if (this != &img_p) {
            nx = img_p.nx;
            ny = img_p.ny;
            nc = img_p.nc;
            bps = img_p.bps;

            size_t bufsiz;
            switch (bps) {
                case 8: {
                    bufsiz = nx*ny*nc*sizeof (unsigned char);
                    break;
                }
                case 16: {
                    bufsiz = nx*ny*nc*sizeof (unsigned short);
                    break;
                }
                default: {
                    bufsiz = 0;
                }
            }
            if (bufsiz > 0) {
                pixels = new byte[bufsiz];
                memcpy (pixels, img_p.pixels, bufsiz);
            }
            xmp = img_p.xmp;
            icc = img_p.icc;
            iptc = img_p.iptc;
            exif = img_p.exif;
            skip_metadata = img_p.skip_metadata;
            conobj = img_p.conobj;
        }
        return *this;
    }
    //============================================================================

    /*!
     * This function compares the actual mime type of a file (based on its magic number) to
     * the given mime type (sent by the client) and the extension of the given filename (sent by the client)
     */
    bool SipiImage::checkMimeTypeConsistency(const std::string &path, const std::string &given_mimetype, const std::string &filename)
    {
        try
        {
            string actual_mimetype = shttps::GetMimetype::getMimetype(path).first;

            if (actual_mimetype != given_mimetype) {
                //std::cerr << actual_mimetype << " does not equal " << given_mimetype << std::endl;
                return false;
            }

            size_t pos = filename.find_last_of(".");
            if (pos == std::string::npos) {
                //std::cerr << "invalid filename " << filename << std::endl;
                return false;
            }

            std::string extension = filename.substr(pos+1);

            std::string mime_from_extension = Sipi::SipiImage::mimetypes.at(extension);

            if (mime_from_extension != actual_mimetype) {
                //std::cerr << "filename " << filename << "has not mime type " << actual_mimetype << std::endl;
                return false;
            }
        }
        catch (std::out_of_range& e)
        {
            // file extension was not found in map
            //std::cerr << "unsupported file type " << filename << std::endl;
            return false;
        }

        return true;
    }

    void SipiImage::read(std::string filepath, SipiRegion *region, SipiSize *size) {
        size_t pos = filepath.find_last_of('.');
        string fext = filepath.substr(pos + 1);
        string _fext;

        bool got_file = false;
        _fext.resize(fext.size());
        std::transform(fext.begin(), fext.end(), _fext.begin(), ::tolower);
        if ((_fext == "tif") || (_fext == "tiff")) {
            got_file = io[string("tif")]->read(this, filepath, region, size);
        }
        else if ((_fext == "jpg") || (_fext == "jpeg")) {
            got_file = io[string("jpg")]->read(this, filepath, region, size);
        }
        else if (_fext == "png") {
            got_file = io[string("png")]->read(this, filepath, region, size);
        }
        else if ((_fext == "jp2") || (_fext == "jpx") || (_fext == "j2k")) {
            got_file = io[string("jpx")]->read(this, filepath, region, size);
        }

        if (!got_file) {
            for(auto const &iterator : io) {
                if ((got_file = iterator.second->read(this, filepath, region, size))) break;
            }
        }
        if (!got_file) {
            throw SipiError(__file__, __LINE__, "Could not read file \"" + filepath + "\"!");
        }
    }
    //============================================================================

    void SipiImage::getDim(string filepath, int &width, int &height) {
        size_t pos = filepath.find_last_of('.');
        string fext = filepath.substr(pos + 1);
        string _fext;

        bool got_file = false;
        _fext.resize(fext.size());
        std::transform(fext.begin(), fext.end(), _fext.begin(), ::tolower);
        if ((_fext == "tif") || (_fext == "tiff")) {
            got_file = io[string("tif")]->getDim(filepath, width, height);
        }
        else if ((_fext == "jpg") || (_fext == "jpeg")) {
            got_file = io[string("jpg")]->getDim(filepath, width, height);
        }
        else if (_fext == "png") {
            got_file = io[string("png")]->getDim(filepath, width, height);
        }
        else if ((_fext == "jp2") || (_fext == "jpx")) {
            got_file = io[string("jpx")]->getDim(filepath, width, height);
        }

        if (!got_file) {
            for(auto const &iterator : io) {
                if ((got_file = iterator.second->getDim(filepath, width, height))) break;
            }
        }
        if (!got_file) {
            throw SipiError(__file__, __LINE__, "Could not read file \"" + filepath + "\"!");
        }
    }
    //============================================================================

    void SipiImage::write(std::string ftype, std::string filepath, int quality) {
        if (quality == -1) {
            io[ftype]->write(this, filepath, 80);
        }
        else {
            io[ftype]->write(this, filepath, quality);
        }
    }
    //============================================================================

    void SipiImage::convertToIcc(const SipiIcc &target_icc_p, int bps) {
        cmsUInt32Number in_formatter, out_formatter;
        if (icc == NULL) {
            switch (nc) {
                case 1: {
                    icc = new SipiIcc(icc_GRAY_D50); // assume gray value image with D50
                    break;
                }
                case 3: {
                    icc = new SipiIcc(icc_sRGB); // assume sRGB
                    break;
                }
                case 4: {
                    icc = new SipiIcc(icc_CYMK_standard); // assume CYMK
                    break;
                }
                default: {
                    throw SipiError(__file__, __LINE__, "Cannot assign ICC profile to image with nc=" + to_string(nc) + "!");
                }
            }
        }

        unsigned int nnc = cmsChannelsOf(cmsGetColorSpace(target_icc_p.getIccProfile()));

        byte *inbuf = (byte *) pixels;
        byte *outbuf = new byte[nx*ny*nnc];

        cmsHTRANSFORM hTransform;
        in_formatter = icc->iccFormatter(this);
        out_formatter = target_icc_p.iccFormatter(bps);
        switch (bps) {
            case 8: {
                hTransform = cmsCreateTransform(icc->getIccProfile(), in_formatter, target_icc_p.getIccProfile(), out_formatter, INTENT_PERCEPTUAL, 0);
                break;
            }
            case 16: {
                hTransform = cmsCreateTransform(icc->getIccProfile(), in_formatter, target_icc_p.getIccProfile(), out_formatter, INTENT_PERCEPTUAL, 0);
                break;
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsuported bits/sample (" + to_string(bps) + ")!");
            }
        }
        cmsDoTransform(hTransform, inbuf, outbuf, nx*ny);

        cmsDeleteTransform(hTransform);

        delete icc;
        icc = new SipiIcc(target_icc_p);

        pixels = outbuf;
        delete [] inbuf;
        nc = nnc;

        PredefinedProfiles targetPT = target_icc_p.getProfileType();
        switch (targetPT) {
            case icc_GRAY_D50: {
                photo = MINISBLACK;
                break;
            }
            case icc_RGB:
            case icc_sRGB:
            case icc_AdobeRGB: {
                photo = RGB;
                break;
            }
            case icc_CYMK_standard: {
                photo = SEPARATED;
                break;
            }
            default: {
                // do nothing at the moment
            }
        }
    }
    /*==========================================================================*/



    void SipiImage::removeChan(unsigned int chan) {
        if ((nc == 1) || (chan >= nc)) {
            string msg = "Cannot remove component!  nc=" + to_string(nc) + " chan=" + to_string(chan);
            throw SipiError(__file__, __LINE__, msg);
        }
        if (bps == 8) {
            byte *inbuf = (byte *) pixels;
            unsigned int nnc = nc - 1;
            byte *outbuf = new byte[nnc*nx*ny];
            for (unsigned int j = 0; j < ny; j++) {
                for (unsigned int i = 0; i < nx; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        if (k == chan) continue;
                        outbuf[nnc*(j*nx + i) + k] = inbuf[nc*(j*nx + i) + k];
                    }
                }
            }
            pixels = outbuf;
            delete [] inbuf;
        }
        else if (bps == 16) {
            word *inbuf = (word *) pixels;
            unsigned int nnc = nc - 1;
            unsigned short *outbuf = new unsigned short[nnc*nx*ny];
            for (unsigned int j = 0; j < ny; j++) {
                for (unsigned int i = 0; i < nx; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        if (k == chan) continue;
                        outbuf[nnc*(j*nx + i) + k] = inbuf[nc*(j*nx + i) + k];
                    }
                }
            }
            pixels = (byte *) outbuf;
            delete [] (word *) inbuf;
        }
        else {
            if (bps != 8) {
                string msg = "Bits per sample is not supported for operation: " + to_string(bps);
                throw SipiError(__file__, __LINE__, msg);
            }
        }
    }
    //============================================================================


    bool SipiImage::crop(int x, int y, int width, int height) {
        if (x < 0) {
            width += x;
            x = 0;
        }
        else if (x >= nx) {
            return false;
        }
        if (y < 0) {
            height += y;
            y = 0;
        }
        else if (y >= ny) {
            return false;
        }

        if (width == 0) {
            width = nx - x;
        }
        else if ((x + width) > nx) {
            width = nx - x;
        }
        if (height == 0) {
            height = ny - y;
        }
        else if ((y + height) > ny) {
            height = ny - y;
        }

        if (width < 0) return false;
        if (height < 0) return false;

        if ((x == 0) && (y == 0) && (width == nx) && (height == ny)) return true; //we do not have to crop!!

        if (bps == 8) {
            byte *inbuf = (byte *) pixels;
            byte *outbuf = new byte[width*height*nc];
            for (unsigned int j = 0; j < height; j++) {
                for (unsigned int i = 0; i < width; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*width + i) + k] = inbuf[nc*((j + y)*nx + (i + x)) + k];
                    }
                }
            }
            pixels = outbuf;
            delete [] inbuf;
        }
        else if (bps == 16) {
            word *inbuf = (word *) pixels;
            word *outbuf = new word[width*height*nc];
            for (unsigned int j = 0; j < height; j++) {
                for (unsigned int i = 0; i < width; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*width + i) + k] = inbuf[nc*((j + y)*nx + (i + x)) + k];
                    }
                }
            }
            pixels = (byte *) outbuf;
            delete [] inbuf;
        }
        else {
            // clean up and throw exception
        }
        nx = width;
        ny = height;

        return true;
    }
    //============================================================================


    bool SipiImage::crop(SipiRegion *region) {
        int x, y, width, height;
        if (region->getType() == SipiRegion::FULL) return true; // we do not have to crop;

        region->crop_coords(nx, ny, x, y, width, height);

        if (bps == 8) {
            byte *inbuf = (byte *) pixels;
            byte *outbuf = new byte[width*height*nc];
            for (unsigned int j = 0; j < height; j++) {
                for (unsigned int i = 0; i < width; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*width + i) + k] = inbuf[nc*((j + y)*nx + (i + x)) + k];
                    }
                }
            }
            pixels = outbuf;
            delete [] inbuf;
        }
        else if (bps == 16) {
            word *inbuf = (word *) pixels;
            word *outbuf = new word[width*height*nc];
            for (unsigned int j = 0; j < height; j++) {
                for (unsigned int i = 0; i < width; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*width + i) + k] = inbuf[nc*((j + y)*nx + (i + x)) + k];
                    }
                }
            }
            pixels = (byte *) outbuf;
            delete [] inbuf;
        }
        else {
            // clean up and throw exception
        }
        nx = width;
        ny = height;
        return true;
    }
    //============================================================================


    /****************************************************************************/
#define POSITION(x,y,c,n) ((n)*((y)*nx + (x)) + c)

    byte SipiImage::bilinn (byte buf[], register int nx, register float x, register float y, register int c, register int n)
    {
        register int ix, iy;
        register float rx, ry;
        ix = (int) x;
        iy = (int) y;
        rx = x - (float) ix;
        ry = y - (float) iy;

        if ((rx < 1.0e-5) && (ry < 1.0e-5)) {
            return (buf[POSITION(ix,iy,c,n)]);
        }
        else if (rx < 1.0e-5) {
            return ((byte) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION(ix, (iy + 1),c,n)] * (ry - rx*ry)) + 0.5));
        }
        else if (ry < 1.0e-5) {
            return ((byte) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION((ix + 1),iy,c,n)] * (rx - rx*ry)) + 0.5));
        }
        else {
            return ((byte) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION((ix + 1),iy,c,n)] * (rx - rx*ry) +
                (float) buf[POSITION(ix, (iy + 1),c,n)] * (ry - rx*ry) +
                (float) buf[POSITION((ix + 1),(iy + 1),c,n)] * rx*ry) + 0.5));
        }
    }
    /*==========================================================================*/

    word SipiImage::bilinn (word buf[], register int nx, register float x, register float y, register int c, register int n)
    {
        register int ix, iy;
        register float rx, ry;
        ix = (int) x;
        iy = (int) y;
        rx = x - (float) ix;
        ry = y - (float) iy;

        if ((rx < 1.0e-5) && (ry < 1.0e-5)) {
            return (buf[POSITION(ix,iy,c,n)]);
        }
        else if (rx < 1.0e-5) {
            return ((word) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION(ix, (iy + 1),c,n)] * (ry - rx*ry)) + 0.5));
        }
        else if (ry < 1.0e-5) {
            return ((word) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION((ix + 1),iy,c,n)] * (rx - rx*ry)) + 0.5));
        }
        else {
            return ((word) (((float) buf[POSITION(ix,iy,c,n)] * (1 - rx - ry + rx*ry) +
                (float) buf[POSITION((ix + 1),iy,c,n)] * (rx - rx*ry) +
                (float) buf[POSITION(ix, (iy + 1),c,n)] * (ry - rx*ry) +
                (float) buf[POSITION((ix + 1),(iy + 1),c,n)] * rx*ry) + 0.5));
        }
    }
    /*==========================================================================*/

#undef POSITION


    bool SipiImage::scale(int nnx, int nny) {
        int iix = 1, iiy = 1;
        int nnnx, nnny;

        //
        // if the scaling is less than 1 (that is, the image gets smaller), we first
        // expand it to a integer multiple of the desired size, and then we just
        // avarage the number of pixels. This is the "proper" way of downscale an
        // image...
        //
        if (nnx < nx) {
            while (nnx*iix < nx) iix++;
            nnnx = nnx*iix;
        }
        else {
            nnnx = nnx;
        }
        if (nny < ny) {
            while (nny*iiy < ny) iiy++;
            nnny = nny*iiy;
        }
        else {
            nnny = nny;
        }

        float *xlut = new float[nnnx];
        for (int i = 0; i < nnnx; i++) {
            xlut[i] = (float) (i*(nx - 1)) / (float) (nnnx - 1);
        }

        float *ylut = new float[nnny];
        for (int j = 0; j < nnny; j++) {
            ylut[j] = (float) (j*(ny - 1)) / (float) (nnny - 1);
        }

        if (bps == 8) {
            byte *inbuf = (byte *) pixels;
            byte *outbuf = new byte[nnnx*nnny*nc];
            register float rx, ry;
            for (unsigned int j = 0; j < nnny; j++) {
                ry = ylut[j];
                for (unsigned int i = 0; i < nnnx; i++) {
                    rx = xlut[i];
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*nnnx + i) + k] = bilinn(inbuf, nx, rx, ry, k, nc);
                    }
                }
            }
            pixels = outbuf;
            delete [] inbuf;
        }
        else if (bps == 16) {
            word *inbuf = (word *) pixels;
            word *outbuf = new word[nnnx*nnny*nc];
            register float rx, ry;
            for (unsigned int j = 0; j < nnny; j++) {
                ry = ylut[j];
                for (unsigned int i = 0; i < nnnx; i++) {
                    rx = xlut[i];
                    for (unsigned int k = 0; k < nc; k++) {
                        outbuf[nc*(j*nnnx + i) + k] = bilinn(inbuf, nx, rx, ry, k, nc);
                    }
                }
            }
            pixels = (byte *) outbuf;
            delete [] inbuf;
        }
        else {
            delete[] xlut;
            delete[] ylut;
            return false;
            // clean up and throw exception
        }

        delete[] xlut;
        delete[] ylut;

        //
        // now we have to check if we have to avarage the pixels
        //
        if ((iix > 1) || (iiy > 1)) {
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nnx*nny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            register unsigned int accu = 0;
                            for (unsigned int jj = 0; jj < iiy; jj++) {
                                for (unsigned int ii = 0; ii < iix; ii++) {
                                    accu += inbuf[nc*((iiy*j + jj)*nnnx + (iix*i + ii)) + k];
                                }
                            }
                            outbuf[nc*(j*nnx + i) + k] = accu / (iix*iiy);
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;
            }
            else if (bps == 16) {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nnx*nny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            register unsigned int accu = 0;
                            for (unsigned int jj = 0; jj < iiy; jj++) {
                                for (unsigned int ii = 0; ii < iix; ii++) {
                                    accu += inbuf[nc*((iiy*j + jj)*nnnx + (iix*i + ii)) + k];
                                }
                            }
                            outbuf[nc*(j*nnx + i) + k] = accu / (iix*iiy);
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
        }
        nx = nnx;
        ny = nny;
        return true;
    }
    //============================================================================


    bool SipiImage::rotate(float angle, bool mirror) {
       if (mirror) {
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nx*ny*nc];
                for (unsigned int j = 0; j < ny; j++) {
                    for (unsigned int i = 0; i < nx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nx + i) + k] = inbuf[nc*(j*nx + (nx - i - 1)) + k];
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;
            }
            else if (bps == 16) {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nx*ny*nc];
                for (unsigned int j = 0; j < ny; j++) {
                    for (unsigned int i = 0; i < nx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nx + i) + k] = inbuf[nc*(j*nx + (nx - i - 1)) + k];
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
            else {
                return false;
                // clean up and throw exception
            }
        }

        while (angle < 0.) angle += 360.;
        while (angle >= 360.) angle -= 360.;

        if (angle == 90.) {
            //
            // abcdef     mga
            // ghijkl ==> nhb
            // mnopqr     oic
            //            pjd
            //            qke
            //            rlf
            //
            int nnx = ny;
            int nny = nx;
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*((ny - i - 1)*nx + j) + k];
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;
            }
            else if (bps == 16) {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*((ny - i - 1)*nx + j) + k];
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
            nx = nnx;
            ny = nny;
        }
        else if (angle == 180.) {
            //
            // abcdef     rqponm
            // ghijkl ==> lkjihg
            // mnopqr     fedcba
            //
            int nnx = nx;
            int nny = ny;
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*((ny - j - 1)*nx + (nx - i - 1)) + k];
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;
            }
            else if (bps == 16) {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*((ny - j - 1)*nx + (nx - i - 1)) + k];
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
            nx = nnx;
            ny = nny;
        }
        else if (angle == 270.) {
            //
            // abcdef     flr
            // ghijkl ==> ekq
            // mnopqr     djp
            //            cio
            //            bhn
            //            agm
            //
            int nnx = ny;
            int nny = nx;
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*(i*nx + (nx - j - 1)) + k];
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;
            }
            else if (bps == 16) {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nx*ny*nc];
                for (unsigned int j = 0; j < nny; j++) {
                    for (unsigned int i = 0; i < nnx; i++) {
                        for (unsigned int k = 0; k < nc; k++) {
                            outbuf[nc*(j*nnx + i) + k] = inbuf[nc*(i*nx + (nx - j - 1)) + k];
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
            nx = nnx;
            ny = nny;

        }
        else { // all other angles
            float phi = -M_PI*angle / 180.0;
            float ptx = nx/2. - .5;
            float pty = ny/2. - .5;

            float si = sinf (phi);
            float co = cosf (phi);
            int nnx;
            int nny;
            if ((angle > 0.) && (angle < 90.)) {
                nnx = floor(nx*cosf(-phi) + ny*sinf(-phi) + .5);
                nny = floor(nx*sinf(-phi) + ny*cosf(-phi) + .5);
            }
            else if ((angle > 90.) && (angle < 180.)) {
                nnx = floor(-nx*cosf(-phi) + ny*sinf(-phi) + .5);
                nny = floor(nx*sinf(-phi) - ny*cosf(-phi) + .5);
            }
            else if ((angle > 180.) && (angle < 270.)) {
                nnx = floor(-nx*cosf(-phi) - ny*sinf(-phi) + .5);
                nny = floor(-nx*sinf(-phi) - ny*cosf(-phi) + .5);
            }
            else {
                nnx = floor(nx*cosf(-phi) - ny*sinf(-phi) + .5);
                nny = floor(-nx*sinf(-phi) + ny*cosf(-phi) + .5);
            }

            float pptx = ptx * (float) nnx / (float) nx;
            float ppty = pty * (float) nny / (float) ny;
            if (bps == 8) {
                byte *inbuf = (byte *) pixels;
                byte *outbuf = new byte[nnx*nny*nc];
                byte bg = 0;
                for (int j = 0; j < nny; j++) {
                    for (int i = 0; i < nnx; i++) {
                        register float rx = ((float) i - pptx)*co - ((float) j - ppty)*si + ptx;
                        register float ry = ((float) i - pptx)*si + ((float) j - ppty)*co + pty;
                        if ((rx < 0.0) || (rx >= (float) (nx - 1)) || (ry < 0.0) || (ry >= (float) (ny - 1))) {
                            for (int k = 0; k < nc; k++) {
                                outbuf[nc*(j*nnx + i) + k] = bg;
                            }
                        }
                        else {
                            for (int k = 0; k < nc; k++) {
                                outbuf[nc*(j*nnx + i) + k] = bilinn(inbuf, nx, rx, ry, k, nc);
                            }
                        }
                    }
                }
                pixels = outbuf;
                delete [] inbuf;

            }
            else if (bps == 16)  {
                word *inbuf = (word *) pixels;
                word *outbuf = new word[nnx*nny*nc];
                word bg = 0;
                for (int j = 0; j < nny; j++) {
                    for (int i = 0; i < nnx; i++) {
                        register float rx = ((float) i - pptx)*co - ((float) j - ppty)*si + ptx;
                        register float ry = ((float) i - pptx)*si + ((float) j - ppty)*co + pty;
                        if ((rx < 0.0) || (rx >= (float) (nx - 1)) || (ry < 0.0) || (ry >= (float) (ny - 1))) {
                            for (int k = 0; k < nc; k++) {
                                outbuf[nc*(j*nnx + i) + k] = bg;
                            }
                        }
                        else {
                            for (int k = 0; k < nc; k++) {
                                outbuf[nc*(j*nnx + i) + k] = bilinn(inbuf, nx, rx, ry, k, nc);
                            }
                        }
                    }
                }
                pixels = (byte *) outbuf;
                delete [] inbuf;
            }
            nx = nnx;
            ny = nny;
        }
        return true;
    }
    //============================================================================

    bool SipiImage::add_watermark(string wmfilename) {
        int wm_nx, wm_ny, wm_nc;
    	byte *wmbuf = read_watermark(wmfilename, wm_nx, wm_ny, wm_nc);
        if (wmbuf == NULL) {
            throw SipiError(__file__, __LINE__, "Cannot read watermark file=\"" + wmfilename + "\" !");
        }

        float *xlut = new float[nx];
        float *ylut = new float[ny];
    	for (int i = 0; i < nx; i++) {
    		xlut[i] = (float) (wm_nx*i) / (float) nx;
    	}
    	for (int j = 0; j < ny; j++) {
    		ylut[j] = (float) (wm_ny*j) / (float) ny;
    	}

        if (bps == 8) {
            byte *buf = (byte *) pixels;
            for (unsigned int j = 0; j < ny; j++) {
                for (unsigned int i = 0; i < nx; i++) {
                    byte val = bilinn(wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);
                    for (unsigned int k = 0; k < nc; k++) {
                        register float nval = (buf[nc*(j*nx + i) + k] / 255.)*(1.0F + val / 2550.0F) + val / 2550.0F;
                        buf[nc*(j*nx + i) + k] = (nval > 1.0) ? 255 : floor(nval*255. + .5);
                    }
                }
            }
        }
        else if (bps == 16)  {
            word *buf = (word *) pixels;
            for (unsigned int j = 0; j < ny; j++) {
                for (unsigned int i = 0; i < nx; i++) {
                    for (unsigned int k = 0; k < nc; k++) {
                        byte val = bilinn(wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);
                        register float nval = (buf[nc*(j*nx + i) + k] / 65535.0F)*(1.0F + val / 655350.0F) + val / 352500.F;
                        buf[nc*(j*nx + i) + k] = (nval > 1.0) ? (word) 65535 : (word) floor(nval*65535. + .5);
                    }
                }
            }
        }

    	delete[] wmbuf;

    	return true;
    }
    /*==========================================================================*/


    ostream &operator<< (ostream &outstr, const SipiImage &rhs) {
        outstr << endl << "SipiImage with the following parameters:" << endl;
        outstr << "nx  = " << to_string(rhs.nx) << endl;
        outstr << "ny  = " << to_string(rhs.ny) << endl;
        outstr << "nc  = " << to_string(rhs.nc) << endl;
        outstr << "bps = " << to_string(rhs.bps) << endl;
        if (rhs.xmp) {
            outstr << "XMP-Metadata: " << endl
                << *(rhs.xmp) << endl;
        }
        if (rhs.iptc) {
            outstr << "IPTC-Metadata: " << endl
                << *(rhs.iptc) << endl;
        }
        if (rhs.exif) {
            outstr << "EXIF-Metadata: " << endl
                << *(rhs.exif) << endl;
        }
        if (rhs.icc) {
            outstr << "ICC-Metadata: " << endl
                << *(rhs.icc) << endl;
        }
        return outstr;
    }
    //============================================================================

}
