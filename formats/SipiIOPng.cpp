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
 */#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdio.h>
#include <math.h>


#include "SipiError.h"
#include "SipiIOPng.h"


#include <png.h>
#include <zlib.h>

static const char __file__[] = __FILE__;


// bad hack in order to include definitions in png.h on debian systems
#if !defined(PNG_TEXT_SUPPORTED)
#  define PNG_TEXT_SUPPORTED 1
#endif
#if !defined(PNG_iTXt_SUPPORTED)
#  define PNG_iTXt_SUPPORTED 1
#endif

using namespace std;

namespace Sipi {

    static char lang_en[] = "en";
    static char exif_tag[] = "Raw profile type exif";
    static char iptc_tag[] = "Raw profile type iptc";
    static char xmp_tag[] = "XML:com.adobe.xmp";

    //============== HELPER CLASS ==================
    class PngTextPtr {
    private:
        png_text *text_ptr;
        unsigned int num_text;
        unsigned int num_text_len;
    public:
        inline PngTextPtr(unsigned int len = 16) : num_text_len(len){num_text = 0; text_ptr = new png_text[num_text_len]; };
        inline ~PngTextPtr() { delete[] text_ptr; };
        inline unsigned int num() {return num_text; };
        inline png_text* ptr() {return text_ptr; };
        png_text *next();
        void add_zTXt(char *key, unsigned char *data, unsigned int len);
        void add_iTXt(char *key, char *data, unsigned int len);
    };

    png_text* PngTextPtr::next() {
        if (num_text < num_text_len) {
            return &(text_ptr[num_text++]);
        }
        else {
            png_text *tmpptr = new png_text[num_text_len + 16];
            for (int i = 0; i < num_text_len; i++) tmpptr[i] = text_ptr[i];
            delete[] text_ptr;
            text_ptr = tmpptr;
            return &(text_ptr[num_text++]);
        }
    }
    //=============================================

    void PngTextPtr::add_zTXt(char *key, unsigned char *data, unsigned int len) {
        png_text *tmp = this->next();
        tmp->compression = PNG_TEXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = (char *) data;
        tmp->text_length = len;
    }
    //=============================================

    void PngTextPtr::add_iTXt(char *key, char *data, unsigned int len) {
        png_text *tmp = this->next();
        tmp->compression = PNG_ITXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = data;
        tmp->itxt_length = len;
    }
    //=============================================


    bool SipiIOPng::read(SipiImage *img, std::string filepath, SipiRegion *region, SipiSize *size)
    {
        FILE *infile;
        unsigned char header[8];
        png_structp png_ptr;
        png_infop info_ptr;
        png_infop end_info;

        //
        // open the input file
        //
        if ((infile = fopen (filepath.c_str(), "rb")) == NULL) {
            return FALSE;
        }
        fread(header, 1, 8, infile);
        if (png_sig_cmp(header, 0, 8) != 0) {
            fclose(infile);
            return FALSE; // it's not a PNG file
        }

        if ((png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, (png_voidp) NULL, (png_error_ptr) NULL, (png_error_ptr) NULL)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_structp !");
        }
        if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_infop !");
        }
        if ((end_info = png_create_info_struct(png_ptr)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_infop !");
        }

        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        img->nx = png_get_image_width(png_ptr, info_ptr);
        img->ny = png_get_image_height(png_ptr, info_ptr);
        img->bps = png_get_bit_depth(png_ptr, info_ptr);
        img->nc = png_get_channels(png_ptr, info_ptr);

        int colortype = png_get_color_type(png_ptr, info_ptr);
        switch (colortype) {
            case PNG_COLOR_TYPE_GRAY: { // implies nc = 1, (bit depths 1, 2, 4, 8, 16)
                img->photo = MINISBLACK;
                break;
            }
            case PNG_COLOR_TYPE_GRAY_ALPHA: { // implies nc = 2, (bit depths 8, 16)
                img->photo = MINISBLACK;
                break;
            }
            case PNG_COLOR_TYPE_PALETTE: { // we will not support it for now, (bit depths 1, 2, 4, 8)
                img->photo = PALETTE;
                break;
            }
            case PNG_COLOR_TYPE_RGB: { // implies nc = 3 (standard case :-), (bit_depths 8, 16)
                img->photo = RGB;
                break;
            }
            case PNG_COLOR_TYPE_RGB_ALPHA: { // implies nc = 4, (bit_depths 8, 16)
                img->photo = RGB;
                break;
            }
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB) != 0) {
            img->icc = new SipiIcc(icc_sRGB);
            cerr << "PNG with sRGB profile: " << filepath << endl;
        }
        else {
            png_charp name;
            int compression_type;
            png_bytep profile;
            png_uint_32 proflen;
            if (png_get_iCCP(png_ptr, info_ptr, &name, &compression_type, &profile, &proflen) != 0) {
                img->icc = new SipiIcc((unsigned char *) profile, (int) proflen);
                cerr << "PNG with ICC profile: " << filepath << endl;
            }
        }

        png_text *png_texts;
        int num_comments = png_get_text(png_ptr, info_ptr, &png_texts, NULL);
        for (int i = 0; i < num_comments; i++) {
            if (strcmp(png_texts[i].key, xmp_tag) == 0) {
                img->xmp = new SipiXmp((char *) png_texts[i].text, (int) png_texts[i].text_length);
            }
            else if (strcmp(png_texts[i].key, exif_tag) == 0) {
                img->exif = new SipiExif((unsigned char *) png_texts[i].text, (unsigned int) png_texts[i].text_length);
            }
            else if (strcmp(png_texts[i].key, iptc_tag) == 0) {
                img->iptc = new SipiIptc((unsigned char *) png_texts[i].text, (unsigned int) png_texts[i].text_length);
            }
            else {
                fprintf(stderr, "PNG-COMMENT: key=\"%s\" text=\"%s\"\n", png_texts[i].key, png_texts[i].text);
            }
        }

        int sll = png_get_rowbytes(png_ptr, info_ptr);
        if (colortype == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
            img->nc = 3;
            img->photo = RGB;
            sll = 3*sll;
        }
        if (colortype == PNG_COLOR_TYPE_GRAY && img->bps < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            img->bps = 8;
        }

        uint8 *buffer = new uint8[img->ny*sll];
        png_bytep *row_pointers = new png_bytep[img->ny];
        for (int i = 0; i < img->ny; i++) {
            row_pointers[i] = (buffer + i*sll);
        }

        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, end_info);
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        if (colortype == PNG_COLOR_TYPE_PALETTE) {
            img->nc = 3;
            img->photo = RGB;
        }

        img->pixels = buffer;

        delete [] row_pointers;

        fclose(infile);

        if (region != NULL) { //we just use the image.crop method
            (void) img->crop(region);
        }

        //
        // resize/Scale the image if necessary
        //
        if (size != NULL) {
            int nnx, nny, reduce;
            bool redonly;
            SipiSize::SizeType rtype = size->get_size(img->nx, img->ny, nnx, nny, reduce, redonly);
            if (rtype != SipiSize::FULL) {
                img->scale(nnx, nny);
            }
        }

        return true;
    };
    /*==========================================================================*/


    bool SipiIOPng::getDim(std::string filepath, int &width, int &height) {
        FILE *infile;
        unsigned char header[8];

        //
        // open the input file
        //
        if ((infile = fopen (filepath.c_str(), "rb")) == NULL) {
            return FALSE;
        }
        fread(header, 1, 8, infile);
        if (png_sig_cmp(header, 0, 8) != 0) {
            fclose(infile);
            return FALSE; // it's not a PNG file
        }

        png_structp png_ptr;
        png_infop info_ptr;
        png_infop end_info;

        if ((png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, (png_voidp) NULL, (png_error_ptr) NULL, (png_error_ptr) NULL)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_structp !");
        }
        if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_infop !");
        }
        if ((end_info = png_create_info_struct(png_ptr)) == NULL) {
            fclose (infile);
            throw SipiError(__file__, __LINE__, "Error reading PNG file \"" + filepath + "\": Could not allocate mempry fpr png_infop !");
        }

        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, 8);


        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);

        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        return true;
    }
    /*==========================================================================*/


    void create_text_chunk(PngTextPtr *png_textptr, char *key, char *str, unsigned int len) {
        png_text *chunk = png_textptr->next();
        chunk->compression = PNG_TEXT_COMPRESSION_NONE;
        chunk->key = key;
        chunk->text = str;
        chunk->text_length = len;
        chunk->itxt_length = 0;
        chunk->lang = lang_en;
        chunk->lang_key = NULL;
    }
    /*==========================================================================*/

    static void conn_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
        shttps::Connection *conn = (shttps::Connection *) png_get_io_ptr(png_ptr);
        conn->sendAndFlush(data, length);
    }
    /*==========================================================================*/

    static void conn_flush_data(png_structp png_ptr) {
        shttps::Connection *conn = (shttps::Connection *) png_get_io_ptr(png_ptr);
        conn->flush();
    }
    /*==========================================================================*/

    void SipiIOPng::write(SipiImage *img, std::string filepath, int quality) {
        FILE *outfile = NULL;
        png_structp png_ptr;
        shttps::Connection *conobj = img->connection();

        if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
            throw SipiError(__file__, __LINE__, "Error writing PNG file \"" + filepath + "\": png_create_write_struct failed !");
        }

        if (strcmp (filepath.c_str(), "-") == 0) {
            outfile = stdout;
        }
        else if (filepath == "HTTP") {
           png_set_write_fn(png_ptr, img->connection(), conn_write_data, conn_flush_data);
            /*
             //png_set_write_fn();
             png_set_write_fn(png_structp write_ptr,
                     voidp write_io_ptr, png_rw_ptr write_data_fn,
                     png_flush_ptr output_flush_fn);

             void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length);

             void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length);

             void user_flush_data(png_structp png_ptr);

             */
        }
        else {
            if (!(outfile = fopen(filepath.c_str(), "wb"))) {
                throw SipiError(__file__, __LINE__, "Error writing PNG file \"" + filepath + "\": Could notopen output file !");
            }
        }

        png_infop info_ptr;
        if (!(info_ptr = png_create_info_struct(png_ptr))) {
            throw SipiError(__file__, __LINE__, "Error writing PNG file \"" + filepath + "\": png_create_info_struct !");
        }

    	if (outfile != NULL) png_init_io(png_ptr, outfile);

    	png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

    	/* set the zlib compression level */
    	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

        int color_type;
        if (img->nc == 1) { // grey value
            color_type = PNG_COLOR_TYPE_GRAY;
        }
        else if (img->nc == 2) { // grey value with alpha
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        }
        else if (img->nc == 3) { // RGB
            color_type = PNG_COLOR_TYPE_RGB;
        }
        else if (img->nc == 4) { // RGB + ALPHA
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        }
        else {
            throw SipiError(__file__, __LINE__, "Error writing PNG file \"" + filepath + "\": cannot handle number of channels () !");
        }

        png_set_IHDR(png_ptr, info_ptr, img->nx, img->ny, img->bps, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        //
        // ICC profile handfling is special...
        //
        unsigned char *icc_buf;
        if (img->icc != NULL) {
            unsigned int len;
            icc_buf = img->icc->iccBytes(len);
            png_set_iCCP(png_ptr, info_ptr, "ICC", PNG_COMPRESSION_TYPE_BASE, icc_buf, len);
        }

        //
        // other metadata comes here
        //
        unsigned char *exif_buf = NULL;
        PngTextPtr *chunk_ptr = new PngTextPtr(4);
        if (img->exif) {
            unsigned int len;
            exif_buf = img->exif->exifBytes(len);
            chunk_ptr->add_zTXt(exif_tag, exif_buf, len);
        }

        unsigned char *iptc_buf = NULL;
        if (img->iptc) {
            unsigned int len;
            iptc_buf = img->iptc->iptcBytes(len);
            chunk_ptr->add_zTXt(iptc_tag, iptc_buf, len);
        }

        char *xmp_buf = NULL;
        if (img->xmp != NULL) {
            unsigned int len;
            xmp_buf = img->xmp->xmpBytes(len);
            chunk_ptr->add_iTXt(xmp_tag, xmp_buf, len);
        }
        if (chunk_ptr->num() > 0) {
            png_set_text(png_ptr, info_ptr, chunk_ptr->ptr(), chunk_ptr->num());
        }

        png_bytep *row_pointers = (png_bytep *) png_malloc (png_ptr, img->ny * sizeof (png_byte *));
        if (img->bps == 8) {
            for (int i = 0; i < img->ny; i++) {
                //png_byte *row = (png_byte *) png_malloc(png_ptr, sizeof(unsigned char) * img->nx * img->nc);
                //memcpy (row, img->pixels + i*img->nx*img->nc, img->nx*img->nc);
                row_pointers[i] = (img->pixels + i*img->nx*img->nc);
                //row_pointers[i] = row;
            }
        }
        else if (img->bps == 16) {
            for (int i = 0; i < img->ny; i++) {
                row_pointers[i] = (img->pixels + 2*i*img->nx*img->nc);
            }
        }

        png_set_rows(png_ptr, info_ptr, row_pointers);

        png_write_info(png_ptr, info_ptr);
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
        png_write_end(png_ptr, info_ptr);

        png_free (png_ptr, row_pointers);

        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

        if (icc_buf != NULL) delete[] icc_buf;
        if (exif_buf != NULL) delete[] exif_buf;
        if (iptc_buf != NULL) delete[] iptc_buf;
        if (xmp_buf != NULL) delete[] xmp_buf;

        if (outfile != NULL) fclose(outfile);
    };

}
