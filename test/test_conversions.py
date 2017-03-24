# Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
# Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
# This file is part of Sipi.
# Sipi is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Sipi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# Additional permission under GNU AGPL version 3 section 7:
# If you modify this Program, or any covered work, by linking or combining
# it with Kakadu (or a modified version of that library) or Adobe ICC Color
# Profiles (or a modified version of that library) or both, containing parts
# covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
# or both, the licensors of this Program grant you additional permission
# to convey the resulting work.
# See the GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public
# License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.

import pytest

# Tests file conversions.

class TestConversions:

    reference_tif_tmpl = "iso-15444-4/reference_jp2/jp2_{}.tif"
    reference_jp2_tmpl = "iso-15444-4/testfiles_jp2/file{}.jp2"
    sipi_jp2_tmpl = "sipi_file{}.jp2"
    sipi_tif_tmpl = "sipi_jp2_{}.tif"
    sipi_round_trip_tmpl = "sipi_sipi_jp2_{}.tif"

    def test_iso_15444_4(self, manager):
        """encode and decode reference images from ISO/IEC 15444-4"""

        results = "\n"
        bad_result = False

        # Skip:
        # - file 2 (https://github.com/dhlab-basel/Sipi/issues/151)
        # - file 3 (https://github.com/dhlab-basel/Sipi/issues/152)
        # - file 4 (https://github.com/dhlab-basel/Sipi/issues/144)
        # - file 6 (https://github.com/dhlab-basel/Sipi/issues/153)
        # - file 8 (https://github.com/dhlab-basel/Sipi/issues/154)
        # - file 9 (https://github.com/dhlab-basel/Sipi/issues/145)

        for i in [1, 5, 7]:
            reference_tif = manager.data_dir_path(self.reference_tif_tmpl.format(i))
            reference_jp2 = manager.data_dir_path(self.reference_jp2_tmpl.format(i))
            sipi_jp2_filename = self.sipi_jp2_tmpl.format(i)
            sipi_tif_filename = self.sipi_tif_tmpl.format(i)
            sipi_round_trip_tif_filename = self.sipi_round_trip_tmpl.format(i)

            tif_to_jp2_result = manager.convert_and_compare(reference_tif, sipi_jp2_filename, reference_jp2)
            converted_jp2 = tif_to_jp2_result["converted_file_path"]
            results += "Image {}: Reference tif -> jp2 (compare with reference jp2)\n    From: {}\n    To: {}\n    Compare with: {}\n    PAE: {}\n\n".format(i, reference_tif, converted_jp2, reference_jp2, tif_to_jp2_result["pae"])

            if tif_to_jp2_result["pae"] != "0 (0)":
                bad_result = True

            jp2_to_tif_result = manager.convert_and_compare(reference_jp2, sipi_tif_filename, reference_tif)
            results += "Image {}: Reference jp2 -> tif (compare with reference tif)\n    From: {}\n    To: {}\n    Compare with: {}\n    PAE: {}\n\n".format(i, reference_jp2, jp2_to_tif_result["converted_file_path"], reference_tif, jp2_to_tif_result["pae"])

            if jp2_to_tif_result["pae"] != "0 (0)":
                bad_result = True

            round_trip_result = manager.convert_and_compare(converted_jp2, sipi_round_trip_tif_filename, reference_tif)
            results += "Image {}: Converted jp2 -> tif (compare with reference tif)\n    From: {}\n    To: {}\n    Compare with: {}\n    PAE: {}\n\n".format(i, converted_jp2, round_trip_result["converted_file_path"], reference_tif, round_trip_result["pae"])

            if round_trip_result["pae"] != "0 (0)":
                bad_result = True


        assert not bad_result, results

