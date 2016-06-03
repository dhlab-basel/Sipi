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
 *//*!
 * This file handles  IIIF Size parameters
 */
#ifndef __sipi_size_h
#define __sipi_size_h

#include <string>

namespace Sipi {

   /*!
    * \class SipiSize
    * This class handles both the Size (or scale) parameter of a IIIF-request, but also deals
    * with the reduce parameter which can be used in reading J2K images
    * In order to support the reduce, the syntax "red:int" can be used, e.g.:
    * http://{url}/{prefix}/{identifier}/{region}/red:3/{rotation}/default.jpg
    * The class can be constructed in several ways and just stores the parameters.
    * The width/height that should be applied to the scaling is calculated by calling
    * the get_size() method. It is to note that get_size() may also return a reduce
    * factor. This may be used in case of the JPEG2000 where it es much more efficient
    * to have the reduce factor while reading the image (since using a reduce > 0 means
    * that only a fraction of the input file has to be read)
    *
    */
    class SipiSize {
    public:
        typedef enum {
            FULL,      //!< The full size of the image is served, IIIF: "full"
            PIXELS_XY, //!< Both X and Y is given, the image may be distorted, IIIF: "xxx,yyy"
            PIXELS_X,  //!< Only the X dimension is given, the Y dimension is calculated (no distortion), IIIF: "xxx,"
            PIXELS_Y,  //!< Only the Y dimension is given, the X dimension is calculated (no distortion), IIIF: ",yyy"
            MAXDIM,    //!< Either X or Y dimensions are taken, so that the resulting image fits in the rectangle (no distortion), IIIF: "!xxx,yyy"
            PERCENTS,  //!< The percentage of scaling is given, IIIF: "pct:ppp"
            REDUCE     //!< A reduce factor can be given, 0=no scaling, 1=0.5, 2=0.25, 3=0.125,...(Note: this is an extension to the IIIF standard) IIIF: "red:ii"
        } SizeType;
    private:
        SizeType size_type; //!< Holds the type of size/scaling parameters given
        float percent;      //!< if the scaling is given in percent, this holds the value
        int reduce;         //!< if the scaling is given by a reduce value
        bool redonly;       //!< we *only* have a reduce in the resulting size
        int nx, ny;         //!< the parameters given
        int w, h;           //!< the resulting width and height after processing
        bool canonical_ok;

    public:
       /*!
        * Default constructor (full size)
        */
        inline SipiSize() { size_type = SizeType::FULL; }

       /*!
        * Constructor with reduce parameter (reduce=0: full image, reduce=1: 1/2, reduce=2: 1/4,…)
        *
        * \param[in] reduce_p Reduce parameter
        */
        inline SipiSize(int reduce_p) : reduce(reduce_p) { size_type = SizeType::REDUCE; }

       /*!
        * Constructor with percentage parameter
        *
        * \param[in] percent_p Percentage parameter
        */
        inline SipiSize(float percent_p) : percent(percent_p) { size_type = SizeType::PERCENTS; }

       /*!
        * Constructor with dimensions. IF one of the dimensions is less-equal zero, this dimension
        * will be calculated from the other without distorting the image. If both dimensions are gt 0,
        * the image may be distorted
        *
        * \param[in] nx_p Width of scaled image
        * \param[in] ny_p Height of scaled image
        * \param[in] maxdim If true, the scaled image (without distortion) will fit into the given region
        */
        SipiSize(int nx_p, int ny_p, bool maxdim = false);

       /*!
        * Construcor taking size/scale part of IIIF url as parameter
        *
        * \param[in] str String with the IIIF url part containing the size/scaling information
        */
        SipiSize(std::string str);

        /*!
         * Comparison operater ">"
         */
        bool operator >(const SipiSize &s);

        /*!
         * Comparison operater ">="
         */
        bool operator >=(const SipiSize &s);

        /*!
         * Comparison operater "<"
         */
        bool operator <(const SipiSize &s);

        /*!
         * Comparison operater "<="
         */
        bool operator <=(const SipiSize &s);

       /*!
        * Get the coordinate type that has bee used for construction of the region
        *
        * \returns CoordType
        */
        inline SizeType getType() { return size_type; };

       /*!
        * Get the size to which the image should be scaled
        *
        * \param[out] nx_p Width of scaled image
        * \param[out] nx_p Height of scaled image
        * \param[out] percent_p Percentage of scaling (if percentage was indicated)
        *
        * \returns enum SizeType which indicates how the size was specified
        */
       /*
        inline SizeType get_size(int &nx_p, int &ny_p, float &percent_p) {
            nx_p = nx; ny_p = ny; percent_p = percent;
            return size_type;
        };
        */
       /*!
        * Get the size to which the image should be scaled
        *
        * \param[in] nx Width of original images
        * \param[in] ny original height of image
        * \param[out] w_p Width of scaled image
        * \param[out] h_p Height of scaled image
        * \param[out] reduce_p Reduce parameter (especially for J2K images with resolution pyramid)
        * \param[out] rdonly_p True, if scaling can be made with the reduce parameter only
        *
        * \returns enum SizeType which indicates how the size was specified
        */
        SipiSize::SizeType get_size(int nx, int ny, int &w_p, int &h_p, int &reduce_p, bool &redonly_p);

       /*!
        * Returns the canoncial IIIF string for the given size/scaling
        *
        * \param[in] Pointer to character buffer
        * \param[in] Length of the character buffer
        */
        void canonical(char *buf, int buflen);

        friend std::ostream &operator<< (std::ostream &lhs, const SipiSize &rhs);
    };

}

#endif
