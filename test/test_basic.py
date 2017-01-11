import pytest

class TestBasic:

    def test_sipi_starts(self, manager):
        """start"""
        assert manager.sipi_is_running()

    def test_sipi_log_output(self, manager):
        """add routes"""
        assert "Added route" in manager.get_sipi_output()

    def test_file_bytes(self, manager):
        """return an unmodified JPG file containing the correct bytes"""
        assert manager.compare("Leaves.jpg/full/full/0/default.jpg", "Leaves.jpg")
