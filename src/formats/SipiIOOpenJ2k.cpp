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

#include <stdio.h>

#include "shttps/Connection.h"
#include "shttps/Global.h"

#include "SipiError.h"
#include "SipiIOJ2k.h"

#include "SipiIOOpenJ2k.h"
#include "shttps/Logger.h"  // logging...

#include "openjpeg.h"

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

static const char __file__[] = __FILE__;

namespace Sipi {

    static void error_callback(const char *msg, void *client_data) {
        (void)client_data;
        fprintf(stdout, "[ERROR] %s", msg);
    }

    static void warning_callback(const char *msg, void *client_data) {
        (void)client_data;
        fprintf(stdout, "[WARNING] %s", msg);
    }

    static void info_callback(const char *msg, void *client_data) {
        (void)client_data;
        fprintf(stdout, "[INFO] %s", msg);
    }



    bool SipiIOOpenJ2k::read(SipiImage *img, string filepath, SipiRegion *region, SipiSize *size)
    {
        auto logger = Logger::getLogger(shttps::loggername);
        FILE *reader;


        opj_dparameters_t parameters;

        opj_set_default_decoder_parameters(&parameters);

        if ((reader = fopen(filepath.c_str(), "rb")) == nullptr) {
            throw SipiError(__file__, __LINE__, "Couldn't open input file!", errno);
        };
        unsigned char buf[12];

        memset(buf, 0, 12);
        ssize_t n = fread(buf, 1, 12, reader);
        fclose(reader);
        if (n != 12) {
            throw SipiError(__file__, __LINE__, "Couldn't determine filetype!");
        }
        if (memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(buf, JP2_MAGIC, 4) == 0) {
            parameters.decod_format = JP2_CFMT;
        }
        else if (memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0) {
            parameters.decod_format = J2K_CFMT;
        }
        else {
            throw SipiError(__file__, __LINE__, "J2K magic not supported!");
        }

        //
        // first we open a file stream
        //
        opj_stream_t *l_stream;
        if (!(l_stream = opj_stream_create_default_file_stream(filepath.c_str(), true))) {
            throw SipiError(__file__, __LINE__, "OpenJPEG2000: Could not create file stream...!");
        }

        opj_codec_t* l_codec;
        switch(parameters.decod_format) {
            case J2K_CFMT:    /* JPEG-2000 codestream */
            {
                /* Get a decoder handle */
                l_codec = opj_create_decompress(OPJ_CODEC_J2K);
                break;
            }
            case JP2_CFMT:    /* JPEG 2000 compressed image data */
            {
                /* Get a decoder handle */
                l_codec = opj_create_decompress(OPJ_CODEC_JP2);
                break;
            }
            case JPT_CFMT:    /* JPEG 2000, JPIP */
            {
                /* Get a decoder handle */
                l_codec = opj_create_decompress(OPJ_CODEC_JPT);
                break;
            }
            default: {
                fprintf(stderr, "skipping file..\n");
                throw SipiError(__file__, __LINE__, "OpenJPEG2000: Invalid format for decompressor!");
            }
        }

        /* catch events using our callbacks and give a local context */
        opj_set_info_handler(l_codec, info_callback,00);
        opj_set_warning_handler(l_codec, warning_callback,00);
        opj_set_error_handler(l_codec, error_callback,00);

        /* Setup the decoder decoding parameters using user parameters */
        if ( !opj_setup_decoder(l_codec, &parameters) ){
            opj_stream_destroy(l_stream);
            opj_destroy_codec(l_codec);
            throw SipiError(__file__, __LINE__, "OpenJPEG2000: failed to setup the decoder!");
        }

        opj_image_t *image = nullptr;
        /* Read the main header of the codestream and if necessary the JP2 boxes*/
        if(! opj_read_header(l_stream, l_codec, &image)){
            opj_stream_destroy(l_stream);
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            throw SipiError(__file__, __LINE__, "OpenJPEG2000: failed to read the header!");
        }
        int __nx, __ny;
        __nx = image->x1;
        __ny = image->y1;

        cerr << "image.x0: " << image->x0 << endl;
        cerr << "image.y0: " << image->y0 << endl;
        cerr << "image.x1: " << image->x1 << endl;
        cerr << "image.y1: " << image->y1 << endl;
        cerr << "image.numcomps: " << image->numcomps << endl;

        //
        // is there a region of interest defined ? If yes, get the cropping parameters...
        //
        bool do_roi = false;
        if ((region != nullptr) && (region->getType()) != SipiRegion::FULL) {
            int x, y, nx, ny;
            try {
                region->crop_coords(__nx, __ny, x, y, nx, ny);
                do_roi = true;
            }
            catch (Sipi::SipiError &err) {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(image);
                throw err;
            }
            if (!opj_set_decode_area(l_codec, image, x, y, x + nx, y + ny)) {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(image);
                throw SipiError(__file__, __LINE__, "OpenJPEG2000: failed to set the decoded area!");
            }
        }

        parameters.cp_reduce = 5;
        opj_setup_decoder(l_codec, &parameters );

        if (!(opj_decode(l_codec, l_stream, image) && opj_end_decompress(l_codec, l_stream))) {
            opj_destroy_codec(l_codec);
            opj_stream_destroy(l_stream);
            opj_image_destroy(image);
            throw SipiError(__file__, __LINE__, "OpenJPEG2000: failed to decode image!!");
        }

        cerr << "image.x0: " << image->x0 << endl;
        cerr << "image.y0: " << image->y0 << endl;
        cerr << "image.x1: " << image->x1 << endl;
        cerr << "image.y1: " << image->y1 << endl;
        cerr << "image.numcomps: " << image->numcomps << endl;


        exit(0);
    }
    //=============================================================================

    bool SipiIOOpenJ2k::getDim(std::string filepath, int &width, int &height)
    {
        auto logger = Logger::getLogger(shttps::loggername);

    }
    //=============================================================================

    void SipiIOOpenJ2k::write(SipiImage *img, string filepath, int quality)
    {
        auto logger = Logger::getLogger(shttps::loggername);

    }
    //=============================================================================
}
