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

#include "Connection.h"
#include "Global.h"

#include "SipiError.h"
#include "SipiIOJ2k.h"

#include "spdlog/spdlog.h"  // logging...


// Kakadu core includes
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
// Application level includes
#include "kdu_file_io.h"
#include "kdu_stripe_decompressor.h"
#include "kdu_stripe_compressor.h"
#include "jp2.h"
#include "jpx.h"

using namespace kdu_core;
using namespace kdu_supp;
using namespace std;

static const char __file__[] = __FILE__;

namespace Sipi {

    //=========================================================================
    // Here we are implementing a subclass of kdu_core::kdu_compressed_target
    // in order to write directly to the mongoose webserver connection
    //
    class J2kHttpStream: public kdu_core::kdu_compressed_target {
    private:
        shttps::Connection *conobj;
    public:
        J2kHttpStream(shttps::Connection *conobj_p);
        ~J2kHttpStream();
        inline int get_capabilities() { return KDU_TARGET_CAP_SEQUENTIAL; };
        inline bool start_rewrite(kdu_long backtrack) { return false; };
        inline bool end_rewrite() { return false; };
        bool write( const kdu_byte * buf, int num_bytes);
        inline void set_target_size( kdu_long num_bytes) {}; // we just ignore it
    };
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Constructor which takes the mongoose connection as parameter
    //........................................................................
    J2kHttpStream::J2kHttpStream(shttps::Connection *conobj_p)
        : kdu_core::kdu_compressed_target()
    {
        conobj = conobj_p;
    };
    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------
    // Distructor which cleans up !!!!!!!!!! We still have to determine what has to be cleaned up!!!!!!!!!!
    //........................................................................
    J2kHttpStream::~J2kHttpStream() {
        // cleanup everything thats necessary...
    };
    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------
    // Write the data to the HTTP connection of mongoose
    //........................................................................
    bool J2kHttpStream::write( const kdu_byte * buf, int num_bytes)
    {
        try {
            conobj->sendAndFlush(buf, num_bytes);
        }
        catch (int i) {
            return false;
        }
        return true;
    };
    //-------------------------------------------------------------------------

    static kdu_core::kdu_byte xmp_uuid[]    = {0xBE, 0x7A, 0xCF, 0xCB, 0x97, 0xA9, 0x42, 0xE8, 0x9C, 0x71, 0x99, 0x94, 0x91, 0xE3, 0xAF, 0xAC};
    static kdu_core::kdu_byte iptc_uuid[]   = {0x33, 0xc7, 0xa4, 0xd2, 0xb8, 0x1d, 0x47, 0x23, 0xa0, 0xba, 0xf1, 0xa3, 0xe0, 0x97, 0xad, 0x38};
    static kdu_core::kdu_byte exif_uuid[]   = {'J', 'p', 'g', 'T', 'i', 'f', 'f', 'E', 'x', 'i', 'f', '-', '>', 'J', 'P', '2'};
    //static kdu_core::kdu_byte geojp2_uuid[] = {0xB1, 0x4B, 0xF8, 0xBD, 0x08, 0x3D, 0x4B, 0x43, 0xA5, 0xAE, 0x8C, 0xD7, 0xD5, 0xA6, 0xCE, 0x03};
    //static kdu_core::kdu_byte world_uuid[] = {0x96, 0xa9, 0xf1, 0xf1, 0xdc, 0x98, 0x40, 0x2d, 0xa7, 0xae, 0xd6, 0x8e, 0x34, 0x45, 0x18, 0x09};

    class kdu_stream_message : public kdu_core::kdu_message {
        public: // Member classes
        kdu_stream_message(std::ostream *stream) {
            this->stream = stream;
        }

        void put_text(const char *string) {
            (*stream) << string;
        }

        void flush(bool end_of_message=false) {
            stream->flush();
        }
        private: // Data
        std::ostream *stream;
    };

    static kdu_stream_message cout_message(&std::cout);
    static kdu_stream_message cerr_message(&std::cerr);
    static kdu_core::kdu_message_formatter pretty_cout(&cout_message);
    static kdu_core::kdu_message_formatter pretty_cerr(&cerr_message);

    static bool is_jpx(const char *fname) {
        FILE *inf;
        int retval = 0;
        if ((inf = fopen(fname, "r")) != NULL) {
            char testbuf[48];
            char sig0[] = {'\xff', '\x52'};
            char sig1[] = {'\xff', '\x4f', '\xff',  '\x51'};
            char sig2[] = {'\x00', '\x00', '\x00', '\x0C', '\x6A', '\x50', '\x20', '\x20', '\x0D', '\x0A', '\x87', '\x0A'};
            int n = fread(testbuf, 1, 48, inf);
            if ((n >= 47) && (memcmp(sig0, testbuf + 45, 2) == 0)) retval = 1;
            if ((n >= 4) && (memcmp(sig1, testbuf, 4) == 0)) retval = 1;
            if ((n >= 12) && (memcmp(sig2, testbuf, 12) == 0)) retval = 1;
        }
        fclose(inf);
        return (retval == 1) ? true : false;
    }
    //=============================================================================


    bool SipiIOJ2k::read(SipiImage *img, string filepath, SipiRegion *region, SipiSize *size, bool force_bps_8) {
        auto logger = spdlog::get(shttps::loggername);

        if (!is_jpx(filepath.c_str())) return false; // It's not a JPGE2000....

        int num_threads;
        if ((num_threads = kdu_get_num_processors()) < 2) num_threads = 0;

        // Custom messaging services
        kdu_customize_warnings(&pretty_cout);
        kdu_customize_errors(&pretty_cerr);

        kdu_core::kdu_compressed_source *input = NULL;
        kdu_supp::kdu_simple_file_source file_in;

        kdu_supp::jp2_family_src jp2_ultimate_src;
        kdu_supp::jpx_source jpx_in;
        kdu_supp::jpx_codestream_source jpx_stream;
        kdu_supp::jpx_layer_source jpx_layer;

        kdu_supp::jp2_channels channels;
        kdu_supp::jp2_palette palette;
        kdu_supp::jp2_resolution resolution;
        kdu_supp::jp2_colour colour;

        jp2_ultimate_src.open(filepath.c_str());

        if (jpx_in.open(&jp2_ultimate_src, true) < 0) { // if < 0, not compatible with JP2 or JPX.  Try opening as a raw code-stream.
            jp2_ultimate_src.close();
            file_in.open(filepath.c_str());
            input = &file_in;
        }
        else {
            jp2_input_box box;
            if (box.open(&jp2_ultimate_src)) {
                do {
                    if (box.get_box_type() == jp2_uuid_4cc) {
                        kdu_byte buf[16];
                        box.read(buf, 16);
                        if (memcmp(buf, xmp_uuid, 16) == 0) {
                            unsigned int len = box.get_remaining_bytes();
                            char *buf = new char[len];
                            box.read((kdu_byte *) buf, len);
                            try {
                                img->xmp = new SipiXmp(buf, len);
                            }
                            catch(SipiError &err) {
                                logger != NULL ? logger << err : cerr << err;
                            }
                            delete [] buf;
                        }
                        else if (memcmp(buf, iptc_uuid, 16) == 0) {
                            unsigned int len = box.get_remaining_bytes();
                            unsigned char *buf = new unsigned char[len];
                            box.read((kdu_byte *) buf, len);
                            try {
                                img->iptc = new SipiIptc(buf, len);
                            }
                            catch(SipiError &err) {
                                logger != NULL ? logger << err : cerr << err;
                            }
                            delete [] buf;
                        }
                        else if (memcmp(buf, exif_uuid, 16) == 0) {
                            unsigned int len = box.get_remaining_bytes();
                            unsigned char *buf = new unsigned char[len];
                            box.read((kdu_byte *) buf, len);
                            try {
                                img->exif = new SipiExif(buf, len);
                            }
                            catch(SipiError &err) {
                                logger != NULL ? logger << err : cerr << err;
                            }
                            delete [] buf;
                        }
                    }
                    box.close();
                } while(box.open_next());
            }


            int stream_id = 0;
            jpx_stream = jpx_in.access_codestream(stream_id);
            input = jpx_stream.open_stream();
        }

        kdu_core::kdu_codestream codestream;
        codestream.create(input);
        //codestream.set_fussy(); // Set the parsing error tolerance.
        codestream.set_fast(); // No errors expected in input

        //
        // get the size of the full image (without reduce!)
        //
        siz_params *siz = codestream.access_siz();
        int __nx, __ny;
        siz->get(Ssize, 0, 0, __ny);
        siz->get(Ssize, 0, 1, __nx);

        //
        // is there a region of interest defined ? If yes, get the cropping parameters...
        //
        kdu_core::kdu_dims roi;
        bool do_roi = false;
        if ((region != NULL) && (region->getType()) != SipiRegion::FULL) {
            try {
                region->crop_coords(__nx, __ny, roi.pos.x, roi.pos.y, roi.size.x, roi.size.y);
                do_roi = true;
            }
            catch (Sipi::SipiError &err) {
                codestream.destroy();
                input->close();
                jpx_in.close(); // Not really necessary here.
                throw err;
            }
        }

        //
        // here we prepare tha scaling/reduce stuff...
        //
        bool do_size = false;
        int reduce = 0;
        int nnx, nny;
        bool redonly = true; // we assume that only a reduce is necessary
        if ((size != NULL) && (size->getType() != SipiSize::FULL)) {
            if (do_roi) {
                size->get_size(roi.size.x, roi.size.y, nnx, nny, reduce, redonly);
            }
            else {
                size->get_size(__nx, __ny, nnx, nny, reduce, redonly);
            }
            do_size = true;
        }

        if (reduce < 0) reduce = 0;
        codestream.apply_input_restrictions(0, 0, reduce, 0, do_roi ? &roi : NULL);


        // Determine number of components to decompress
        kdu_core::kdu_dims dims;
        codestream.get_dims(0, dims);

        img->nx = dims.size.x;
        img->ny = dims.size.y;


        img->bps = codestream.get_bit_depth(0); // bitdepth of zeroth component. Assuming it's valid for all

        img->nc = codestream.get_num_components();

        //
        // get ICC-Profile if available
        //
        jpx_layer = jpx_in.access_layer(0);
        if (jpx_layer.exists()) {
            kdu_supp::jp2_colour colinfo = jpx_layer.access_colour(0);
            if (colinfo.exists()) {
                int space = colinfo.get_space();
                switch (space) {
                    case kdu_supp::JP2_sRGB_SPACE: {
                        img->photo = RGB;
                        img->icc = new SipiIcc(icc_sRGB);
                        break;
                    }
                    case kdu_supp::JP2_CMYK_SPACE: {
                        img->photo = SEPARATED;
                        img->icc = new SipiIcc(icc_CYMK_standard);
                        break;
                    }
                    case kdu_supp::JP2_YCbCr1_SPACE: {
                        img->photo = YCBCR;
                        img->icc = new SipiIcc(icc_sRGB);
                        break;
                    }
                    case kdu_supp::JP2_YCbCr2_SPACE:
                    case kdu_supp::JP2_YCbCr3_SPACE: {
                        float whitepoint[] = {0.3127, 0.3290};
                        float primaries[] = {0.630,0.340, 0.310,0.595, 0.155,0.070};
                        img->photo = YCBCR;
                        img->icc = new SipiIcc(whitepoint, primaries);
                        break;
                    }
                    case kdu_supp::JP2_iccRGB_SPACE: {
                        img->photo = RGB;
                        int icc_len;
                        const unsigned char *icc_buf = (const unsigned char *) colinfo.get_icc_profile(&icc_len);
                        img->icc = new SipiIcc(icc_buf, icc_len);
                        break;
                    }
                    case kdu_supp::JP2_iccANY_SPACE: {
                        img->photo = RGB;
                        int icc_len;
                        const unsigned char *icc_buf = (const unsigned char *) colinfo.get_icc_profile(&icc_len);
                        img->icc = new SipiIcc(icc_buf, icc_len);
                        break;
                    }
                    default: {
                        throw SipiImageError("Unsupported ICC profile: " + to_string(space));
                    }
                }
            }
        }

        //
        // the following code directly converts a 16-Bit jpx into an 8-bit image.
        // In order to retrieve a 16-Bit image, use kdu_uin16 *buffer an the apropriate signature of the pull_stripe method
        //
        kdu_supp::kdu_stripe_decompressor decompressor;
        decompressor.start(codestream);
        int stripe_heights[4] = {dims.size.y, dims.size.y, dims.size.y, dims.size.y}; // enough for alpha channel (4 components)

        if (force_bps_8) img->bps = 8; // forces kakadu to convert to 8 bit!
        switch (img->bps) {
            case 8: {
                kdu_core::kdu_byte *buffer8 = new kdu_core::kdu_byte[(int) dims.area()*img->nc];
                decompressor.pull_stripe(buffer8, stripe_heights);
                img->pixels = (byte *) buffer8;
                break;
            }
            case 16: {
                bool *get_signed = new bool[img->nc];
                for (int i = 0; i < img->nc; i++) get_signed[i] = FALSE;
                kdu_core::kdu_int16 *buffer16 = new kdu_core::kdu_int16[(int) dims.area()*img->nc];
                decompressor.pull_stripe(buffer16, stripe_heights, NULL, NULL, NULL, NULL, get_signed);
                img->pixels = (byte *) buffer16;
                break;
            }
            default: {
                decompressor.finish();
                codestream.destroy();
                input->close();
                jpx_in.close(); // Not really necessary here.
                throw SipiImageError("Unsupported number of bits/sample!");
            }
        }
        decompressor.finish();
        codestream.destroy();
        input->close();
        jpx_in.close(); // Not really necessary here.

        if ((size != NULL) && (!redonly)) {
            img->scale(nnx, nny);
        }

        return true;
    }
    //=============================================================================


    bool SipiIOJ2k::getDim(std::string filepath, int &width, int &height) {
        if (!is_jpx(filepath.c_str())) return false; // It's not a JPGE2000....

        kdu_supp::jp2_family_src jp2_ultimate_src;
        kdu_supp::jpx_source jpx_in;
        kdu_supp::jpx_codestream_source jpx_stream;
        kdu_core::kdu_compressed_source *input = NULL;
        kdu_supp::kdu_simple_file_source file_in;

        jp2_ultimate_src.open(filepath.c_str());

        if (jpx_in.open(&jp2_ultimate_src, true) < 0) { // if < 0, not compatible with JP2 or JPX.  Try opening as a raw code-stream.
            jp2_ultimate_src.close();
            file_in.open(filepath.c_str());
            input = &file_in;
        }
        else {
            int stream_id = 0;
            jpx_stream = jpx_in.access_codestream(stream_id);
            input = jpx_stream.open_stream();
        }

        kdu_core::kdu_codestream codestream;
        codestream.create(input);
        codestream.set_fussy(); // Set the parsing error tolerance.

        //
        // get the size of the full image (without reduce!)
        //
        siz_params *siz = codestream.access_siz();
        siz->get(Ssize, 0, 0, height);
        siz->get(Ssize, 0, 1, width);

        codestream.destroy();
        input->close();
        jpx_in.close(); // Not really necessary here.

        return true;
    }
    //=============================================================================


    static void write_xmp_box(kdu_supp::jp2_family_tgt *tgt, const char *xmpstr) {
        kdu_supp::jp2_output_box out;
        out.open(tgt, jp2_uuid_4cc);
        out.set_target_size(strlen(xmpstr) + sizeof(xmp_uuid));
        out.write(xmp_uuid, 16);
        out.write((kdu_core::kdu_byte *) xmpstr, strlen(xmpstr));
        out.close();
    }
    //=============================================================================

    static void write_iptc_box(kdu_supp::jp2_family_tgt *tgt, kdu_core::kdu_byte *iptc, int iptc_len) {
        kdu_supp::jp2_output_box out;
        out.open(tgt, jp2_uuid_4cc);
        out.set_target_size(iptc_len + sizeof(iptc_uuid));
        out.write(iptc_uuid, 16);
        out.write((kdu_core::kdu_byte *) iptc, iptc_len);
        out.close();
    }
    //=============================================================================

    static void write_exif_box(kdu_supp::jp2_family_tgt *tgt, kdu_core::kdu_byte *exif, int exif_len) {
        kdu_supp::jp2_output_box out;
        out.open(tgt, jp2_uuid_4cc);
        out.set_target_size(exif_len + sizeof(exif_uuid));
        out.write(exif_uuid, sizeof(exif_uuid));
        out.write((kdu_byte *) exif, exif_len); // NOT::: skip JPEG marker header 'E', 'x', 'i', 'f', '\0', '\0'..
        out.close();
    }
    //=============================================================================


    void SipiIOJ2k::write(SipiImage *img, string filepath, int quality) {
        kdu_customize_warnings(&pretty_cout);
        kdu_customize_errors(&pretty_cerr);
        int num_threads;

        if ((num_threads = kdu_get_num_processors()) < 2) num_threads = 0;

        // Construct code-stream object
        siz_params siz;
        siz.set(Scomponents, 0, 0, img->nc);
        siz.set(Sdims, 0, 0, img->ny);  // Height of first image component
        siz.set(Sdims, 0, 1, img->nx);   // Width of first image component
        siz.set(Sprecision, 0, 0, img->bps);  // Bits per sample (usually 8 or 16)
        siz.set(Ssigned, 0, 0, false); // Image samples are originally unsigned
        kdu_params *siz_ref = &siz;
        siz_ref->finalize();

    	kdu_codestream codestream;

        kdu_compressed_target *output = NULL;
        jp2_family_tgt jp2_ultimate_tgt;
        jpx_target jpx_out;
        jpx_codestream_target jpx_stream;
        jpx_layer_target jpx_layer;
        jp2_dimensions jp2_family_dimensions;
        jp2_palette jp2_family_palette;
        jp2_resolution jp2_family_resolution;
        jp2_channels jp2_family_channels;
        jp2_colour jp2_family_colour;

        J2kHttpStream *http = NULL;
        if (filepath == "HTTP") {
            shttps::Connection *conobj = img->connection();
            http = new J2kHttpStream(conobj);
            jp2_ultimate_tgt.open(http);
        }
        else {
            jp2_ultimate_tgt.open(filepath.c_str());
        }
        jpx_out.open(&jp2_ultimate_tgt);
        jpx_stream = jpx_out.add_codestream();
        jpx_layer = jpx_out.add_layer();

        jp2_family_dimensions = jpx_stream.access_dimensions();
        jp2_family_palette = jpx_stream.access_palette();
        jp2_family_resolution = jpx_layer.access_resolution();
        jp2_family_channels = jpx_layer.access_channels();
        jp2_family_colour = jpx_layer.add_colour();

        output = jpx_stream.access_stream();

        codestream.create(&siz, output);

        kdu_codestream_comment comment = codestream.add_comment();
        comment.put_text("http://rosenthaler.org/sipi/0.9B/");

        // Set up any specific coding parameters and finalize them.

        codestream.access_siz()->parse_string("Creversible=yes");
        codestream.access_siz()->parse_string("Clayers=8");
        codestream.access_siz()->parse_string("Clevels=8");
        codestream.access_siz()->parse_string("Corder=RPCL");
        codestream.access_siz()->parse_string("Cprecincts={256,256}");
        codestream.access_siz()->parse_string("Cblk={64,64}");
        codestream.access_siz()->parse_string("Cuse_sop=yes");
        //codestream.access_siz()->parse_string("Stiles={1024,1024}");
        //codestream.access_siz()->parse_string("ORGgen_plt=yes");
        //codestream.access_siz()->parse_string("ORGtparts=R");
        /*
        codestream.access_siz()->parse_string("Clayers=8");
        codestream.access_siz()->parse_string("Creversible=yes");
*/
        codestream.access_siz()->finalize_all(); // Set up coding defaults
        //codestream.access_siz()->finalize(); // Set up coding defaults

        jp2_family_dimensions.init(&siz); // initalize dimension box

        if (img->icc != NULL) {
            PredefinedProfiles icc_type = img->icc->getProfileType();
            switch (icc_type) {
                case icc_undefined: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                    break;
                }
                case icc_unknown: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                    break;
                }
                case icc_sRGB: {
                    jp2_family_colour.init(JP2_sRGB_SPACE);
                    break;
                }
                case icc_AdobeRGB: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                    break;
                }
                case icc_RGB: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                    break;
                }
                case icc_CYMK_standard: {
                    jp2_family_colour.init(JP2_CMYK_SPACE);
                    break;
                }
                case icc_GRAY_D50: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                }
                default: {
                    unsigned int icc_len;
                    kdu_byte *icc_bytes = (kdu_byte *) img->icc->iccBytes(icc_len);
                    jp2_family_colour.init(icc_bytes);
                }
            }

        }
        else {
            switch (img->nc) {
                case 1: {
                    jp2_family_colour.init(JP2_sLUM_SPACE);
                    break;
                }
                case 3: {
                    jp2_family_colour.init(JP2_sRGB_SPACE);
                    break;
                }
                case 4: {
                    jp2_family_colour.init(JP2_CMYK_SPACE);
                    break;



                }
            }
        }

        jp2_family_channels.init(img->nc);
        for (int c = 0; c < img->nc; c++) jp2_family_channels.set_colour_mapping(c,c);

        jpx_out.write_headers();

        if (img->iptc != NULL) {
            unsigned int iptc_len = 0;
            kdu_byte *iptc_buf = img->iptc->iptcBytes(iptc_len);
            write_iptc_box(&jp2_ultimate_tgt, iptc_buf, iptc_len);
        }

        //
        // write EXIF here
        //
        if (img->exif != NULL) {
            unsigned int exif_len = 0;
            kdu_byte *exif_buf = img->exif->exifBytes(exif_len);
            write_exif_box(&jp2_ultimate_tgt, exif_buf, exif_len);
        }

        //
        // write XMP data here
        //
        if (img->xmp != NULL) {
            unsigned int len = 0;
            const char *xmp_buf = img->xmp->xmpBytes(len);
            if (len > 0) {
                write_xmp_box(&jp2_ultimate_tgt, xmp_buf);
            }
        }

        //jpx_out.write_headers();
        jp2_output_box *out_box = jpx_stream.open_stream();

        out_box->write_header_last(); // don't know if we have to use this. For the mongoose connection it is not allowed.

        codestream.access_siz()->finalize_all();

        kdu_thread_env env, *env_ref = NULL;
        if (num_threads > 0) {
            env.create();
            for (int nt=1; nt < num_threads; nt++) {
                if (!env.add_thread()) num_threads = nt; // Unable to create all the threads requested
            }
            env_ref = &env;
        }


        // Now compress the image in one hit, using `kdu_stripe_compressor'
        kdu_stripe_compressor compressor;
        //compressor.start(codestream);
        compressor.start(codestream, 0, NULL, NULL, 0, false, false, true, 0.0, 0, false, env_ref);

        int *stripe_heights = new int[img->nc];
        for (int i = 0; i < img->nc; i++) {
            stripe_heights[i] = img->ny;
        }

        int *precisions;
        bool *is_signed;

        if (img->bps == 16) {
            kdu_int16 *buf = (kdu_int16 *) img->pixels;
            precisions = new int[img->nc];
            is_signed = new bool[img->nc];
            for (int i = 0; i < img->nc; i++) {
                precisions[i] = img->bps;
                is_signed[i] = false;
            }
            compressor.push_stripe(buf, stripe_heights, NULL, NULL, NULL, precisions, is_signed);
        }
        else if (img->bps == 8){
            kdu_byte *buf = (kdu_byte *) img->pixels;
            compressor.push_stripe(buf, stripe_heights);
        }
    	else {
            throw SipiImageError("Unsupported number of bits/sample!");
        }
        compressor.finish();

        // Finally, cleanup
        codestream.destroy(); // All done: simple as that.
        output->close(); // Not really necessary here.
        jpx_out.close();
        if (jp2_ultimate_tgt.exists()) {
            jp2_ultimate_tgt.close();
        }

        delete [] stripe_heights;
        if (img->bps == 16) {
            delete [] precisions;
            delete [] is_signed;
        }

        delete http;

        return;
    }
} // namespace Sipi
