import pytest
from sipi_test_manager import *

def test_sipi_starts(manager):
    assert manager.sipi_is_running()

def test_sipi_output(manager):
    print(manager.get_sipi_output())
    assert True
