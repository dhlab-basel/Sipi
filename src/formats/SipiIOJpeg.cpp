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
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>

#include <stdio.h>

#include "SipiError.h"
#include "SipiIOJpeg.h"
#include "Connection.h"

#include "jpeglib.h"

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */


using namespace std;

static const char __file__[] = __FILE__;

namespace Sipi {

    /*!
     * Special exception within the JPEG routines which can be caught separately
     */
    class JpegError : public runtime_error {
    public:
        inline JpegError() : runtime_error("!! JPEG_ERROR") {}
        inline JpegError(const char *msg) : runtime_error(msg) {}
        inline const char* what() const noexcept {
            return runtime_error::what();
        }
    };
    //------------------------------------------------------------------

    /*!
     * Struct that is used to hold the variables for defining the
     * private I/O routines which are used to write the the HTTP socket
     */
    typedef struct _HtmlBuffer {
        JOCTET *buffer; //!< Buffer for holding data to be written out
        size_t buflen;  //!< length of the buffer
        shttps::Connection *conobj; //!< Pointer to the connection objects
    } HtmlBuffer;

    /*!
     * Function which initializes the structures for managing the IO
     */
    static void init_destination (j_compress_ptr cinfo) {
        HtmlBuffer *html_buffer = (HtmlBuffer *) cinfo->client_data;
        cinfo->dest->free_in_buffer = html_buffer->buflen;
        cinfo->dest->next_output_byte = html_buffer->buffer;
    }
    //=============================================================================

    /*!
     * Function empty the libjeg buffer and write the data to the socket
     */
    static boolean empty_output_buffer (j_compress_ptr cinfo) {
        HtmlBuffer *html_buffer = (HtmlBuffer *) cinfo->client_data;
       try {
            html_buffer->conobj->sendAndFlush(html_buffer->buffer, html_buffer->buflen);
        }
        catch (int i) { // an error occurred (possibly a broken pipe)
            return false;
        }
        cinfo->dest->free_in_buffer = html_buffer->buflen;
        cinfo->dest->next_output_byte = html_buffer->buffer;

        return true;
    }
    //=============================================================================

    /*!
     * Finish writing data
     */
    static void term_destination (j_compress_ptr cinfo) {
        HtmlBuffer *html_buffer = (HtmlBuffer *) cinfo->client_data;
        size_t nbytes = cinfo->dest->next_output_byte - html_buffer->buffer;
        try {
            html_buffer->conobj->sendAndFlush(html_buffer->buffer, nbytes);
        }
        catch (int i) { // an error occured in sending the data (broken pipe?)
            // do nothing...
        }

        free(html_buffer->buffer);

        free(html_buffer);
        cinfo->client_data = NULL;

        free(cinfo->dest);
        cinfo->dest = NULL;
    }
    //=============================================================================

    /*!
     * This function is used to setup the I/O destination to the HTTP socket
     */
    static void jpeg_html_dest(struct jpeg_compress_struct *cinfo, shttps::Connection *conobj, size_t buflen = 64*1024)
    {
      struct jpeg_destination_mgr *destmgr;
      HtmlBuffer *html_buffer;
      cinfo->client_data = malloc(sizeof(HtmlBuffer));
      html_buffer = (HtmlBuffer *) cinfo->client_data;

      html_buffer->buffer = (JOCTET *) malloc(buflen*sizeof(JOCTET));
      html_buffer->buflen = buflen;
      html_buffer->conobj = conobj;

      destmgr = (struct jpeg_destination_mgr *) malloc(sizeof(struct jpeg_destination_mgr));

      destmgr->init_destination = init_destination;
      destmgr->empty_output_buffer = empty_output_buffer;
      destmgr->term_destination = term_destination;

      cinfo->dest = destmgr;
    }
    //=============================================================================

    void SipiIOJpeg::parse_photoshop(SipiImage *img, char *data, int length) {
        int slen = 0;
        unsigned int datalen = 0;
        char *ptr = data;
        unsigned short id;
        char sig[5];
        char name[256];
        int i;

        //cerr << "Parse photoshop: TOTAL LENGTH = " << length << endl;

        while ((ptr - data) < length) {
            sig[0] = *ptr;
            sig[1] = *(ptr + 1);
            sig[2] = *(ptr + 2);
            sig[3] = *(ptr + 3);
            sig[4] = '\0';
            if (strcmp(sig, "8BIM") != 0) break;
            ptr += 4;

            //
            // tag-ID processing
            id = ((unsigned char)*(ptr + 0) << 8) | (unsigned char)*(ptr + 1); // ID
            ptr += 2; // ID

            //
            // name processing (Pascal string)
            slen = *ptr;
            for (i = 0; (i < slen) && (i < 256); i++) name[i] = *(ptr + i + 1);
            name[i] = '\0';
            slen++; // add length byte
            if ((slen % 2) == 1) slen++;
            ptr += slen;

            //
            // data processing
            datalen = ((unsigned char)*ptr << 24) | ((unsigned char)*(ptr + 1) << 16) | ((unsigned char)*(ptr + 2) << 8) | (unsigned char)*(ptr + 3);

            ptr += 4;

            switch(id) {
                case 0x0404: { // IPTC data
                    //cerr << ">>> Photoshop: IPTC" << endl;
                    if (img->iptc == NULL) img->iptc = new SipiIptc((unsigned char *) ptr, datalen);
                    // IPTC – handled separately!
                    break;
                }
                case 0x040f: { // ICC data
                    //cerr << ">>> Photoshop: ICC" << endl;
                    // ICC profile
                    if (img->icc == NULL) img->icc = new SipiIcc((unsigned char *) ptr, datalen);
                    break;
                }
                case 0x0422: { // EXIF data
                    if (img->exif == NULL) img->exif = new SipiExif((unsigned char *) ptr, datalen);
                    //cerr << ">>> Photoshop: EXIF" << endl;
                    // exif
                    break;
                }
                case 0x0424: { // XMP data
                    //cerr << ">>> Photoshop: XMP" << endl;
                    // XMP data
                    if (img->xmp == NULL)img->xmp = new SipiXmp(ptr, datalen);
                }
                default: {
                    // URL
                    char *str = (char *) calloc(1, (datalen + 1)*sizeof(char));
                    memcpy(str, ptr, datalen);
                    str[datalen] = '\0';
                    //fprintf(stderr, "XXX=%s\n", str);
                    break;
                }
            }

            if ((datalen % 2) == 1) datalen++;
            ptr += datalen;
        }
    }
    //=============================================================================


    /*!
    * This function is used to catch libjpeg errors which otherwise would
    * result in a call exit()
    */
    static void jpegErrorExit ( j_common_ptr cinfo )
    {
        char jpegLastErrorMsg[JMSG_LENGTH_MAX];
        /* Create the message */
        ( *( cinfo->err->format_message ) ) ( cinfo, jpegLastErrorMsg );

        /* Jump to the setjmp point */
        throw JpegError(jpegLastErrorMsg);
    }
    //=============================================================================


    bool SipiIOJpeg::read(SipiImage *img, std::string filepath, SipiRegion *region, SipiSize *size) {
        FILE *infile;

        //
        // open the input file
        //
        if ((infile = fopen (filepath.c_str(), "rb")) == NULL) {
            return false;
        }

        // workaround for bug #0011: jpeglib crashes the app when the file is not a jpeg file
        // we check the magic number before calling any jpeglib routines
        int magic1 = fgetc(infile);
        int magic2 = fgetc(infile);

        if ((magic1 != 0xff) || (magic2 != 0xd8)) {
            fclose (infile);
            return false; // it's not a JPEG file!
        }

        // move infile position back to the beginning of the file
        rewind(infile);
        // end of workaround for bug #0011

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;


        JSAMPARRAY linbuf = NULL;
        jpeg_saved_marker_ptr marker;

        //
        // let's create the decompressor
        //
        jpeg_create_decompress (&cinfo);

        cinfo.dct_method = JDCT_FLOAT;


        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = jpegErrorExit;

        jpeg_stdio_src(&cinfo, infile);

        try {
            jpeg_save_markers(&cinfo, JPEG_COM, 0xffff);
            for (int i = 0; i < 16; i++) {
                jpeg_save_markers(&cinfo, JPEG_APP0 + i, 0xffff);
            }
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }

        //
        // now we read the header
        //
        int res;
        try  {
            res = jpeg_read_header (&cinfo, TRUE);
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }
        if (res != JPEG_HEADER_OK) {
            jpeg_destroy_decompress(&cinfo);
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\"");
        }

        //
        // getting Metadata
        //
        marker = cinfo.marker_list;
        unsigned char *icc_buffer = NULL;
        int icc_buffer_len = 0;
        while (marker) {
            //fprintf(stderr, "#######################################################################\n");
            if (marker->marker == JPEG_COM) {
                //fprintf(stderr, "Comment, length %ld:\n", (long) marker->data_length);
            }
            else if (marker->marker == JPEG_APP0+1) { // EXIF, XMP MARKER....
                //
                // first we try to find the exif part
                //
                unsigned char *pos = (unsigned char *) memmem(marker->data, marker->data_length, "Exif\000\000", 6);
                if (pos != NULL) {
                    img->exif = new SipiExif(pos + 6, marker->data_length - (pos - marker->data) - 6);
                }

                //
                // first we try to find the xmp part: TODO: reading XMP which spans multiple segments. See ExtendedXMP !!!
                //
                pos = (unsigned char *) memmem(marker->data, marker->data_length, "http://ns.adobe.com/xap/1.0/\000", 29);
                if (pos != NULL) {
                    char start[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'b', 'e', 'g', 'i', 'n', '\0'};
                    char end[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'e', 'n', 'd', '\0'};

                    char *s;
                    unsigned int ll = 0;
                    do {
                        s = start;
                        while ((ll < marker->data_length) && (*pos != *s)) {
                            pos++; //// ISSUE: code failes here if there are many concurrent access; data overrrun??
                            ll++;
                        }
                        while ((ll < marker->data_length) && (*s != '\0') && (*pos == *s)) {
                            pos++;
                            s++;
                            ll++;
                        }
                    } while (*s != '\0');
                    while ((ll < marker->data_length) && (*pos != '>')) {
                        ll++;
                        pos++;
                    }
                    pos++; // finally we have the start of XMP string
                    unsigned char *start_xmp = pos;

                    unsigned char *end_xmp;
                    do {
                        s = end;
                        while (*pos != *s) pos++;
                        end_xmp = pos; // a candidate
                        while ((*s != '\0') && (*pos == *s)) {
                            pos++;
                            s++;
                        }
                    } while (*s != '\0');
                    while (*pos != '>') pos++;
                    pos++;

                    unsigned int xmp_len = end_xmp - start_xmp;
                    img->xmp = new SipiXmp((char *) start_xmp, xmp_len);
                }
            }
            else if (marker->marker == JPEG_APP0+2) { // ICC MARKER.... may span multiple marker segments
                //
                // first we try to find the exif part
                //
                unsigned char *pos = (unsigned char *) memmem(marker->data, marker->data_length, "ICC_PROFILE\0", 12);
                if (pos != NULL) {
                    int len = marker->data_length - (pos - (unsigned char *) marker->data) - 14;
                    icc_buffer = (unsigned char *) realloc(icc_buffer, icc_buffer_len + len);
                    memcpy (icc_buffer + icc_buffer_len, pos + 14, len);
                    icc_buffer_len += len;
                }
            }
            else if (marker->marker == JPEG_APP0+13) { // PHOTOSHOP MARKER....
                if (strncmp("Photoshop 3.0", (char *) marker->data, 14) == 0) {
                    parse_photoshop(img, (char *) marker->data + 14, (int) marker->data_length - 14);
                }
            }
            else {
                //fprintf(stderr, "4) MARKER= %d, %d Bytes, ==> %s\n\n", marker->marker - JPEG_APP0, marker->data_length, marker->data);
            }
            marker = marker->next;
        }
        if (icc_buffer != NULL) {
            img->icc = new SipiIcc(icc_buffer, icc_buffer_len);
        }

        try {
            jpeg_start_decompress(&cinfo);
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }

        img->bps = 8;
        img->nx = cinfo.output_width;
        img->ny = cinfo.output_height;
        img->nc = cinfo.output_components;
        int colspace = cinfo.out_color_space; // JCS_UNKNOWN, JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr, JCS_CMYK, JCS_YCCK
        switch (colspace) {
            case JCS_RGB: {
                img->photo = RGB;
                break;
            }
            case JCS_GRAYSCALE: {
                img->photo = MINISBLACK;
                break;
            }
            case JCS_CMYK: {
                img->photo = SEPARATED;
                break;
            }
            case JCS_YCbCr: {
                img->photo = YCBCR;
                break;
            }
            case JCS_YCCK: {
                throw SipiError(__file__, __LINE__, "Unsupported JPEG colorspace!");
            }
            case JCS_UNKNOWN: {
                throw SipiError(__file__, __LINE__, "Unsupported JPEG colorspace!");
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported JPEG colorspace!");
            }
        }
        int sll = cinfo.output_components*cinfo.output_width*sizeof (uint8);

        img->pixels = new byte[img->ny*sll];

        try {
            linbuf = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, sll, 1);
            for (int i = 0; i < img->ny; i++) {
                jpeg_read_scanlines(&cinfo, linbuf, 1);
                memcpy(&(img->pixels[i * sll]), linbuf[0], sll);
            }
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }
        try {
            jpeg_finish_decompress(&cinfo);
        }

        catch (JpegError &jpgerr) {
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }

        try {
            jpeg_destroy_decompress(&cinfo);
        }
        catch (JpegError &jpgerr) {
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\": " + jpgerr.what());
        }


        fclose(infile);

        //
        // do some croping...
        //
        if ((region != NULL) && (region->getType()) != SipiRegion::FULL) {
            (void) img->crop(region);
        }

        //
        // resize/Scale the image if necessary
        //
        if ((size != NULL) && (size->getType() != SipiSize::FULL)) {
            int nnx, nny, reduce;
            bool redonly;
            SipiSize::SizeType rtype = size->get_size(img->nx, img->ny, nnx, nny, reduce, redonly);
            if (rtype != SipiSize::FULL) {
                img->scale(nnx, nny);
            }
        }

        return TRUE;
    }
    //============================================================================


    bool SipiIOJpeg::getDim(std::string filepath, int &width, int &height) {
        FILE *infile;
        //
        // open the input file
        //
        if ((infile = fopen (filepath.c_str(), "rb")) == NULL) {
            return false;
        }

        // workaround for bug #0011: jpeglib crashes the app when the file is not a jpeg file
        // we check the magic number before calling any jpeglib routines
        int magic1 = fgetc(infile);
        int magic2 = fgetc(infile);

        if ((magic1 != 0xff) || (magic2 != 0xd8)) {
            return false; // it's not a JPEG file!
        }

        // move infile position back to the beginning of the file
        rewind(infile);
        // end of workaround for bug #0011

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        //
        // let's create the decompressor
        //
        jpeg_create_decompress (&cinfo);

        cinfo.dct_method = JDCT_FLOAT;
        cinfo.err = jpeg_std_error (&jerr);


        cinfo.dct_method = JDCT_FLOAT;
        cinfo.err = jpeg_std_error (&jerr);

        jpeg_stdio_src (&cinfo, infile);

        //
        // now we read the header
        //
        if (jpeg_read_header (&cinfo, TRUE) != JPEG_HEADER_OK) {
            throw SipiError(__file__, __LINE__, "Error reading JPEG file: \"" + filepath + "\"");
        }
        jpeg_start_decompress (&cinfo);

        width = cinfo.output_width;
        height = cinfo.output_height;
        jpeg_destroy_decompress (&cinfo);

        fclose (infile);
        return true;
    }
    //============================================================================




    void SipiIOJpeg::write(SipiImage *img, std::string filepath, int quality) {
        //
        // TODO! Support incoming 16 bit images by converting the buffer to 8 bit!
        //
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error( &jerr );
        jerr.error_exit = jpegErrorExit;

        FILE *outfile = NULL;		/* target file */
        JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
        int row_stride;		/* physical row width in image buffer */

        try {
            jpeg_create_compress(&cinfo);
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_compress(&cinfo);
            throw SipiImageError(jpgerr.what());
        }
        if (strcmp (filepath.c_str(), "HTTP") == 0) { // we are transmitting the data through the webserver
            shttps::Connection *conobj = img->connection();
            jpeg_html_dest(&cinfo, conobj);
        }
        else {
            if (strcmp (filepath.c_str(), "-") == 0) {
                outfile = stdout;
            }
            else {
                if ((outfile = fopen(filepath.c_str(), "wb")) == NULL) {
                    throw SipiError(__file__, __LINE__, "Cannot open file \"" + filepath + "\"!");
                }
            }
            jpeg_stdio_dest(&cinfo, outfile);
        }

        cinfo.image_width = img->nx; 	/* image width and height, in pixels */
        cinfo.image_height = img->ny;
        cinfo.input_components = img->nc;		/* # of color components per pixel */
        switch (img->photo) {
            case MINISWHITE:
            case MINISBLACK: {
                if (img->nc != 1) throw SipiError(__file__, __LINE__, "Num of components not 1 (nc = " + to_string(img->nc) + ")!");
                cinfo.in_color_space = JCS_GRAYSCALE;
                cinfo.jpeg_color_space = JCS_GRAYSCALE;
                break;
            }
            case RGB: {
                if (img->nc != 3) throw SipiError(__file__, __LINE__, "Num of components not 3 (nc = " + to_string(img->nc) + ")!");
                cinfo.in_color_space = JCS_RGB;
                cinfo.jpeg_color_space = JCS_RGB;
                break;
            }
            case SEPARATED: {
                if (img->nc != 4) throw SipiError(__file__, __LINE__, "Num of components not 3 (nc = " + to_string(img->nc) + ")!");
                cinfo.in_color_space = JCS_CMYK;
                cinfo.jpeg_color_space = JCS_CMYK;
                break;
            }
            case YCBCR: {
                if (img->nc != 3) throw SipiError(__file__, __LINE__, "Num of components not 3 (nc = " + to_string(img->nc) + ")!");
                cinfo.in_color_space = JCS_YCbCr;
                cinfo.jpeg_color_space = JCS_YCbCr;
                break;
            }
            default: {
                throw SipiError(__file__, __LINE__, "Unsupported JPEG colorspace!");
            }
        }
        cinfo.progressive_mode = TRUE;
        cinfo.write_Adobe_marker = TRUE;
        cinfo.write_JFIF_header = TRUE;

        try {
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, quality, TRUE /* TRUE, then limit to baseline-JPEG values */);

            jpeg_simple_progression(&cinfo);
            jpeg_start_compress(&cinfo, TRUE);
        }
        catch (JpegError &jpgerr) {
            jpeg_finish_compress (&cinfo);
            jpeg_destroy_compress(&cinfo);
            if (outfile != NULL) fclose(outfile);
            throw SipiImageError(jpgerr.what());
        }




        //
        // Here we write the marker
        //
        //
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // ATTENTION: The markers must be written in the right sequence: APP0, APP1, APP2, ..., APP15
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //

        if (img->exif != NULL) {
            unsigned int len;
            unsigned char *buf = img->exif->exifBytes(len);
            char start[] = "Exif\000\000";
            size_t start_l = sizeof(start) - 1;  // remove trailing '\0';
            unsigned char *exifchunk = new unsigned char[len + start_l];
            memcpy(exifchunk, start, start_l);
            memcpy(exifchunk + start_l, buf, len);
            delete [] buf;

            try {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (JOCTET *) exifchunk, start_l + len);
            }
            catch (JpegError &jpgerr) {
                delete [] exifchunk;
                jpeg_finish_compress (&cinfo);
                jpeg_destroy_compress(&cinfo);
                if (outfile != NULL) fclose(outfile);
                throw SipiImageError(jpgerr.what());
            }
            delete [] exifchunk;
        }

        if (img->xmp != NULL) {
            unsigned int len;
            const char *buf = img->xmp->xmpBytes(len);
            char start[] = "http://ns.adobe.com/xap/1.0/\000";
            size_t start_l = sizeof(start) - 1; // remove trailing '\0';
            char *xmpchunk = new char[len + start_l];
            memcpy(xmpchunk, start, start_l);
            memcpy(xmpchunk + start_l, buf, len);
            delete [] buf;
            try {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (JOCTET *) xmpchunk, start_l + len);
            }
            catch (JpegError &jpgerr) {
                delete [] xmpchunk;
                jpeg_finish_compress (&cinfo);
                jpeg_destroy_compress(&cinfo);
                if (outfile != NULL) fclose(outfile);
                throw SipiImageError(jpgerr.what());
            }
            delete [] xmpchunk;
        }

        if (img->icc != NULL) {
            unsigned int len;
            const unsigned char *buf = img->icc->iccBytes(len);
            unsigned char start[14] = {0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x0}; //"ICC_PROFILE\000";
            size_t start_l = 14;
            unsigned int n = len / (65533 - start_l + 1) + 1;

            unsigned char *iccchunk = new unsigned char[65533];

            unsigned int n_towrite = len;
            unsigned int n_nextwrite = 65533 - start_l;
            unsigned int n_written = 0;
            for (unsigned int i = 0; i < n; i++) {
                start[12] = (unsigned char) (i + 1);
                start[13] = (unsigned char) n;
                if (n_nextwrite > n_towrite) n_nextwrite = n_towrite;
                memcpy(iccchunk, start, start_l);
                memcpy(iccchunk + start_l, buf + n_written, n_nextwrite);
                try {
                    jpeg_write_marker(&cinfo, ICC_MARKER, (JOCTET *) iccchunk, n_nextwrite + start_l);
                }
                catch (JpegError &jpgerr) {
                    delete [] buf;
                    delete [] iccchunk;
                    jpeg_finish_compress (&cinfo);
                    jpeg_destroy_compress(&cinfo);
                    if (outfile != NULL) fclose(outfile);
                    throw SipiImageError(jpgerr.what());
                }

                n_towrite -= n_nextwrite;
                n_written += n_nextwrite;
            }
            delete [] buf;
            delete [] iccchunk;
            if (n_towrite != 0) {
                cerr << "Hoppla!" << endl;
            }
        }

        if (img->iptc != NULL) {
            unsigned int len;
            const unsigned char *buf = img->iptc->iptcBytes(len);
            char start[] = "Photoshop 3.0\0008BIM\004\004\000\000";
            size_t start_l = sizeof(start) - 1;
            unsigned char siz[4];
            siz[0] = (unsigned char) ((len >> 24) & 0x000000ff);
            siz[1] = (unsigned char) ((len >> 16) & 0x000000ff);
            siz[2] = (unsigned char) ((len >> 8) & 0x000000ff);
            siz[3] = (unsigned char) (len & 0x000000ff);

            char *iptcchunk = new char[start_l + 4 + len];
            memcpy(iptcchunk, start, start_l);
            memcpy(iptcchunk + start_l, siz, 4);
            memcpy(iptcchunk + start_l + 4, buf, len);

            delete [] buf;
            try {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 13, (JOCTET *) iptcchunk, start_l + len);
            }
            catch (JpegError &jpgerr) {
                delete [] iptcchunk;
                jpeg_destroy_compress(&cinfo);
                if (outfile != NULL) fclose(outfile);
                throw SipiImageError(jpgerr.what());
            }

            delete [] iptcchunk;
        }

        row_stride = img->nx * img->nc;	/* JSAMPLEs per row in image_buffer */

        try {
            while (cinfo.next_scanline < cinfo.image_height) {
                // jpeg_write_scanlines expects an array of pointers to scanlines.
                // Here the array is only one element long, but you could pass
                // more than one scanline at a time if that's more convenient.
                row_pointer[0] = &img->pixels[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_compress(&cinfo);
            if (outfile != NULL) fclose(outfile);
            throw SipiImageError(jpgerr.what());
        }

        try {
            jpeg_finish_compress(&cinfo);
        }
        catch (JpegError &jpgerr) {
            jpeg_destroy_compress(&cinfo);
            if (outfile != NULL) fclose(outfile);
            throw SipiImageError(jpgerr.what());
        }
        if (outfile != NULL) fclose(outfile);

        jpeg_destroy_compress(&cinfo);
    }

} // namespace
