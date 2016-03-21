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
 * This file implements the virtual abstract class which implements the image file I/O.
 */
#ifndef __sipi_io_h
#define __sipi_io_h

#include "SipiImage.h"
#include "iiifparser/SipiRegion.h"
#include "iiifparser/SipiSize.h"

/**
 * @namespace Sipi Is used for all Sipi things.
 */
namespace Sipi {


    class SipiImage; //!< forward declaration of class SipiImage

   /*!
    * This is the virtual base class for all classes implementing image I/O.
    */
    class SipiIO {
    public:
        /*!
         * Method used to read an image file
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Image file path
         * \param reduce Reducing factor. Some file formars support
         * reading a lower resolution of the file. A reducing factor of 2 indicates
         * to read only half the resolution. [default: 0]
         */
        virtual bool read(SipiImage *img, std::string filepath, SipiRegion *region = NULL, SipiSize *size = NULL) = 0;

       /*!
        * Get the dimension of the image
        *
        * \param[in] filepath Pathname of the image file
        * \param[out] width Width of the image in pixels
        * \param[out] height Height of the image in pixels
        */
        virtual bool getDim(std::string filepath, int &width, int &height) = 0;

        /*!
         * Write an image for a file using the given file format implemented by the subclass
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Name of the image file to be written. Please note that
         * - "-" means to write the image data to stdout
         * - "HTTP" means to write the image data to the HTTP-server output
         */
        virtual void write(SipiImage *img, std::string filepath, int quality = 0) = 0;
    };

}

#endif
