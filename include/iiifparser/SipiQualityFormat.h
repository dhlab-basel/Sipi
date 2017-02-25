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
#ifndef SIPI_SIPIQUALITYFORMAT_H
#define SIPI_SIPIQUALITYFORMAT_H

#include <string>


namespace Sipi {
    class SipiQualityFormat {
    public:
        typedef enum {
            DEFAULT, COLOR, GRAY, BITONAL
        } QualityType;
        typedef enum {
            UNSUPPORTED, JPG, TIF, PNG, GIF, JP2, PDF, WEBP
        } FormatType;

    private:
        QualityType quality_type;
        FormatType format_type;

    public:
        inline SipiQualityFormat() {
            quality_type = SipiQualityFormat::DEFAULT;
            format_type = SipiQualityFormat::JPG;
        }

        SipiQualityFormat(std::string str);

        friend std::ostream &operator<<(std::ostream &lhs, const SipiQualityFormat &rhs);

        inline QualityType quality() { return quality_type; };

        inline FormatType format() { return format_type; };
    };

}

#endif //SIPI_SIPIQUALITYFORMAT_H
