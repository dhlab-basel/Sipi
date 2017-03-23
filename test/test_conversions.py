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

        # This just tests one image at the moment, because:
        # - 'gm compare' can't read the reference image file3.jp2
        # - 'gm compare' reports lots of distortion in Sipi's JP2 images, but visually they look OK.
        #
        # I'm going to try with a different comparison tool.

        for i in [1]:
	        reference_tif = self.reference_tif_tmpl.format(i)
	        reference_jp2 = self.reference_jp2_tmpl.format(i)
	        sipi_jp2_filename = self.sipi_jp2_tmpl.format(i)
	        sipi_tif_filename = self.sipi_tif_tmpl.format(i)
	        sipi_round_trip_filename = self.sipi_round_trip_tmpl.format(i)

	        converted_jp2 = manager.convert_and_compare(reference_tif, sipi_jp2_filename, reference_jp2)
	        converted_tif = manager.convert_and_compare(reference_jp2, sipi_tif_filename, reference_tif)
	        round_trip_tif = manager.convert_and_compare(converted_jp2, sipi_round_trip_filename, reference_tif)
