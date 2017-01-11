import pytest

class TestBasic:

    def test_sipi_starts(self, manager):
        '''start'''
        assert manager.sipi_is_running()

    def test_sipi_output(self, manager):
        '''add routes'''
        assert 'Added route' in manager.get_sipi_output()
