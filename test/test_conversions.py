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

# Tests image format conversions using the Sipi server.

class TestConversions:

    def test_tif8_to_jpg(self, manager):
        """convert Leaves8.tif to a JPG file containing the expected number of bytes"""
        manager.compare("Leaves8.tif/full/full/0/default.jpg", "Leaves8.jpg", just_size=True)

    def test_tif8_to_png(self, manager):
        """convert Leaves8.tif to a PNG file containing the expected number of bytes"""
        manager.compare("Leaves8.tif/full/full/0/default.png", "Leaves8.png", just_size=True)

    def test_tif8_to_jp2(self, manager):
        """convert Leaves8.tif to a JP2 file containing the expected bytes"""
        manager.compare("Leaves8.tif/full/full/0/default.jp2", "Leaves8.jp2")

    def test_tif16_to_jpg(self, manager):
        """convert Leaves16.tif to a JPG file containing the expected number of bytes"""
        manager.compare("Leaves16.tif/full/full/0/default.jpg", "Leaves16.jpg", just_size=True)

    def test_tif16_to_png(self, manager):
        """convert Leaves16.tif to a PNG file containing the expected bytes"""
        manager.compare("Leaves16.tif/full/full/0/default.png", "Leaves16.png")

    def test_tif16_to_jp2(self, manager):
        """convert Leaves16.tif to a JP2 file containing the expected bytes"""
        manager.compare("Leaves16.tif/full/full/0/default.jp2", "Leaves16.jp2")

    def test_jp2c_to_jpg(self, manager):
        """convert LeavesC.jp2 to a JPG file containing the expected number of bytes"""
        manager.compare("LeavesC.jp2/full/full/0/default.jpg", "LeavesC.jpg", just_size=True)

    def test_jp2c_to_png(self, manager):
        """convert LeavesC.jp2 to a PNG file containing the expected bytes"""
        manager.compare("LeavesC.jp2/full/full/0/default.png", "LeavesC.png")

    def test_jp2r_to_jpg(self, manager):
        """convert LeavesR.jp2 to a JPG file containing the expected number of bytes"""
        manager.compare("LeavesR.jp2/full/full/0/default.jpg", "LeavesR.jpg", just_size=True)

    def test_jp2r_to_png(self, manager):
        """convert LeavesR.jp2 to a PNG file containing the expected bytes"""
        manager.compare("LeavesR.jp2/full/full/0/default.png", "LeavesR.png")
