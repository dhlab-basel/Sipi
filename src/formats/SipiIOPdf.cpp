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
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>

#include <stdio.h>

#include "SipiError.h"
#include "SipiIOPdf.h"
#include "SipiCommon.h"
#include "shttps/Connection.h"
#include "shttps/makeunique.h"

#include "podofo/podofo.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    bool SipiIOPdf::read(SipiImage *img, std::string filepath, std::shared_ptr<SipiRegion> region,
              std::shared_ptr<SipiSize> size, bool force_bps_8,
              ScalingQuality scaling_quality) {
        return FALSE;
    };

    SipiImgInfo SipiIOPdf::getDim(std::string filepath) {
        SipiImgInfo info;
        return info;
    };

    void SipiIOPdf::write(SipiImage *img, std::string filepath, int quality) {
        if (img->bps == 16) img->to8bps();

        //
        // we have to check if the image has an alpha channel (not supported by JPEG). If
        // so, we remove it!
        //
        if ((img->getNc() > 3) && (img->getNalpha() > 0)) { // we have an alpha channel....
            for (size_t i = 3; i < (img->getNalpha() + 3); i++) img->removeChan(i);
        }

        double dScaleX = 1.0;
        double dScaleY = 1.0;
        double dScale  = 1.0;

        PoDoFo::PdfRefCountedBuffer *pdf_buffer = new PoDoFo::PdfRefCountedBuffer(32000);
        PoDoFo::PdfOutputDevice *pdf_dev = new PoDoFo::PdfOutputDevice(pdf_buffer);
        PoDoFo::PdfStreamedDocument document(pdf_dev);

        PoDoFo::PdfRect size = PoDoFo::PdfRect( 0.0, 0.0, img->nx, img->ny);

        PoDoFo::PdfPainter painter;

        PoDoFo::PdfPage* pPage;
        PoDoFo::PdfImage image( &document );
        switch (img->photo) {
            case MINISWHITE:
            case MINISBLACK: {
                if (img->nc != 1) {
                    throw SipiImageError(__file__, __LINE__,
                                         "Num of components not 1 (nc = " + std::to_string(img->nc) + ")!");
                }
                image.SetImageColorSpace(PoDoFo::ePdfColorSpace_DeviceGray);
                break;
            }
            case RGB: {
                if (img->nc != 3) {
                    throw SipiImageError(__file__, __LINE__,
                                         "Num of components not 3 (nc = " + std::to_string(img->nc) + ")!");
                }
                image.SetImageColorSpace(PoDoFo::ePdfColorSpace_DeviceRGB);
                break;
            }
            case SEPARATED: {
                if (img->nc != 4) {
                    throw SipiImageError(__file__, __LINE__,
                                         "Num of components not 3 (nc = " + std::to_string(img->nc) + ")!");
                }
                image.SetImageColorSpace(PoDoFo::ePdfColorSpace_DeviceCMYK);
                break;
            }
            case YCBCR: {
                if (img->nc != 3) {
                    throw SipiImageError(__file__, __LINE__,
                                         "Num of components not 3 (nc = " + std::to_string(img->nc) + ")!");
                }
                throw SipiImageError(__file__, __LINE__, "Unsupported PDF colorspace: " + std::to_string(img->photo));
               break;
            }
            case CIELAB: {
                image.SetImageColorSpace(PoDoFo::ePdfColorSpace_CieLab);
                break;
            }
            default: {
                throw SipiImageError(__file__, __LINE__, "Unsupported PDF colorspace: " + std::to_string(img->photo));
            }
        }
        PoDoFo::PdfMemoryInputStream inimgstream((const char *) img->pixels, (PoDoFo::pdf_long) img->nx * img->ny * img->nc);
        image.SetImageData	(img->nx, img->ny, 8, &inimgstream);

        pPage = document.CreatePage(size);

        dScaleX = size.GetWidth() / image.GetWidth();
        dScaleY = size.GetHeight() / image.GetHeight();
        dScale  = PoDoFo::PDF_MIN( dScaleX, dScaleY );

        painter.SetPage( pPage );

        if( dScale < 1.0 )
        {
            painter.DrawImage( 0.0, 0.0, &image, dScale, dScale );
        }
        else
        {
            // Center Image
            double dX = (size.GetWidth() - image.GetWidth())/2.0;
            double dY = (size.GetHeight() - image.GetHeight())/2.0;
            painter.DrawImage( dX, dY, &image );
        }
        painter.FinishPage();

        document.GetInfo()->SetCreator ( PoDoFo::PdfString("SIPI-IIIF-Server") );
        document.GetInfo()->SetAuthor  ( PoDoFo::PdfString("Lukas Rosenthaler") );
        document.GetInfo()->SetTitle   ( PoDoFo::PdfString("Hello World") );
        document.GetInfo()->SetSubject ( PoDoFo::PdfString("Testing the PoDoFo PDF Library") );
        document.GetInfo()->SetKeywords( PoDoFo::PdfString("Test;PDF;Hello World;") );
        document.Close();

        if (filepath == "HTTP") { // we are transmitting the data through the webserver
            shttps::Connection *conn = img->connection();
            conn->sendAndFlush(pdf_buffer->GetBuffer(), pdf_buffer->GetSize());
        }
        else {

        }
        delete pdf_dev;
        delete pdf_buffer;
    };

}
