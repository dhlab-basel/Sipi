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
import configparser
import os
import shlex
import signal
import subprocess
import threading
import time
import requests
import tempfile
import filecmp
import shutil
import psutil

@pytest.fixture(scope="session")
def manager():
    """Returns a SipiTestManager. Automatically starts Sipi and nginx before tests are run, and stops them afterwards."""
    manager = SipiTestManager()
    manager.start_nginx()
    manager.start_sipi()
    yield manager
    manager.stop_sipi()
    manager.stop_nginx()


class SipiTestManager:
    """Controls Sipi and Nginx during tests."""

    def __init__(self):
        """Reads config.ini."""

        self.config = configparser.ConfigParser()
        with open(os.path.abspath("config.ini")) as config_file:
            self.config.read_file(config_file)

        self.sipi_working_dir = os.path.abspath("..")

        # Ensure Sipi doesn't use caching in tests.
        sipi_cache_dir = os.path.join(self.sipi_working_dir, "cache")
        if os.path.isdir(sipi_cache_dir):
            shutil.rmtree(sipi_cache_dir)

        sipi_config = self.config["Sipi"]
        self.sipi_config_file = sipi_config["config-file"]
        self.sipi_command = "build/sipi -config config/{}".format(self.sipi_config_file)
        self.data_dir = os.path.abspath(self.config["Test"]["data-dir"])
        self.sipi_port = sipi_config["port"]
        self.sipi_prefix = sipi_config["prefix"]
        self.sipi_base_url = "http://localhost:{}/{}".format(self.sipi_port, self.sipi_prefix)
        self.sipi_ready_output = sipi_config["ready-output"]
        self.sipi_start_wait = int(sipi_config["start-wait"])
        self.sipi_stop_wait = int(sipi_config["stop-wait"])
        self.sipi_process = None
        self.sipi_started = False
        self.sipi_took_too_long = False

        self.nginx_base_url = self.config["Nginx"]["base-url"]
        self.nginx_working_dir = os.path.abspath("nginx")
        self.start_nginx_command = "nginx -p {} -c nginx.conf".format(self.nginx_working_dir)
        self.stop_nginx_command = "nginx -p {} -c nginx.conf -s stop".format(self.nginx_working_dir)

        self.iiif_validator_command = "iiif-validate.py -s localhost:{} -p {} -i 67352ccc-d1b0-11e1-89ae-279075081939.jp2 --version=2.0 -v".format(self.sipi_port, self.sipi_prefix)

        self.compare_command = "compare -metric MAE {} {} null:"

    def start_sipi(self):
        """Starts Sipi and waits until it is ready to receive requests."""

        def check_for_ready_output(line):
            if self.sipi_ready_output in line:
                self.sipi_started = True

        # Stop any existing Sipi process. This could happen if a previous test run crashed.
        for proc in psutil.process_iter():
            try:
                if proc.name() == "sipi":
                    proc.terminate()
                    proc.wait()
            except psutil.NoSuchProcess:
                pass

        self.sipi_process = None
        self.sipi_started = False
        self.sipi_took_too_long = False

        # Start a Sipi process and capture its output.
        sipi_args = shlex.split(self.sipi_command)
        sipi_start_time = time.time()
        self.sipi_process = subprocess.Popen(sipi_args,
            cwd=self.sipi_working_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True)
        self.sipi_output_reader = ProcessOutputReader(self.sipi_process.stdout, check_for_ready_output)

        # Wait until Sipi says it's ready to receive requests.
        while (not self.sipi_started) and (not self.sipi_took_too_long):
            time.sleep(0.2)
            if (time.time() - sipi_start_time > self.sipi_start_wait):
                self.sipi_took_too_long = True

        if self.sipi_took_too_long:
            raise SipiTestError("Sipi didn't start after {} seconds".format(self.sipi_start_wait))

    def stop_sipi(self):
        """Sends SIGTERM to Sipi and waits for it to stop."""

        if (self.sipi_process != None):
            self.sipi_process.send_signal(signal.SIGTERM)
            self.sipi_process.wait(timeout=self.sipi_stop_wait)
            self.sipi_process = None

    def sipi_is_running(self):
        """Returns True if Sipi is running."""

        if (self.sipi_process == None):
            return false
        else:
            return self.sipi_process.poll() == None

    def get_sipi_output(self):
        """Returns the output collected from Sipi"s stdout and stderr."""

        return self.sipi_output_reader.get_output()

    def start_nginx(self):
        """Starts nginx."""

        nginx_log_dir = os.path.join(self.nginx_working_dir, "logs")

        if not os.path.exists(nginx_log_dir):
            os.makedirs(nginx_log_dir)

        # Make sure nginx isn't already running.
        try:
            self.stop_nginx()
        except SipiTestError:
            pass

        nginx_args = shlex.split(self.start_nginx_command)
        if subprocess.run(nginx_args).returncode != 0:
            raise SipiTestError("nginx failed to start")

    def stop_nginx(self):
        """Stops nginx."""
        
        nginx_args = shlex.split(self.stop_nginx_command)
        if subprocess.run(nginx_args).returncode != 0:
            raise SipiTestError("nginx failed to stop")

    def download_file(self, url_path, suffix=None, headers=None):
        """
            Makes an HTTP request and downloads the response content to a temporary file.
            Returns the absolute path of the temporary file.

            url_path: a path that will be appended to the Sipi base URL to make the request.
            suffix: the file extension that should be given to the temporary file.
            headers: an optional dictionary of request headers.
        """

        url = "{}/{}".format(self.sipi_base_url, url_path)
        response = requests.get(url, headers=headers, stream=True)
        response.raise_for_status()
        temp_fd, temp_file_path = tempfile.mkstemp(suffix=suffix)
        temp_file = os.fdopen(temp_fd, mode="wb")

        for chunk in response.iter_content(chunk_size=8192):
            temp_file.write(chunk)

        temp_file.close()
        return temp_file_path

    def compare_bytes(self, url_path, filename, headers=None):
        """
            Downloads a temporary file and compares it with an existing file on disk. If the two are equivalent,
            deletes the temporary file, otherwise raises an exception.

            url_path: a path that will be appended to the Sipi base URL to make the request.
            filename: the name of a file in the test data directory, containing the expected data.
            headers: an optional dictionary of request headers.
        """

        expected_file_path = os.path.join(self.data_dir, filename)
        expected_file_basename, expected_file_extension = os.path.splitext(expected_file_path)
        downloaded_file_path = self.download_file(url_path, headers=headers, suffix=expected_file_extension)

        if filecmp.cmp(downloaded_file_path, expected_file_path):
            os.remove(downloaded_file_path)
        else:
            raise SipiTestError("Downloaded file {} is different from expected file {}".format(downloaded_file_path, expected_file_path))

    def compare_images(self, url_path, filename, headers=None):
        """
            Downloads a temporary image file and compares it with an existing image file on disk, using ImageMagick's
            'compare' program with the mean absolute error metric. If 'compare' finds no distortion, deletes the temporary file,
            otherwise raises an exception.

            url_path: a path that will be appended to the Sipi base URL to make the request.
            filename: the name of a file in the test data directory, containing the expected data.
            headers: an optional dictionary of request headers.
        """

        expected_file_path = os.path.join(self.data_dir, filename)
        expected_file_basename, expected_file_extension = os.path.splitext(expected_file_path)
        downloaded_file_path = self.download_file(url_path, headers=headers, suffix=expected_file_extension)
        compare_process_args = shlex.split(self.compare_command.format(expected_file_path, downloaded_file_path))
        compare_process = subprocess.run(compare_process_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines = True)

        if compare_process.returncode != 0:
            raise SipiTestError("Image comparison failed:\n" + compare_process.stdout)

        if not compare_process.stdout.startswith("0 (0)"):
            raise SipiTestError("Image {} is different from image {}: image distortion is {}".format(downloaded_file_path, expected_file_path, compare_process.stdout))

        os.remove(downloaded_file_path)

    def run_iiif_validator(self):
        """Runs the IIIF validator."""

        validator_process_args = shlex.split(self.iiif_validator_command)
        validator_process = subprocess.run(validator_process_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines = True)

        if validator_process.returncode != 0:
            raise SipiTestError("IIIF validation failed:\n" + validator_process.stdout)


class SipiTestError(Exception):
    """Indicates an error in a Sipi test."""

class ProcessOutputReader:
    """Spawns a thread that collects the output of a subprocess."""

    def __init__(self, stream, line_func):
        self.stream = stream
        self.line_func = line_func
        self.lines = []

        def collect_lines():
            while True:
                line = self.stream.readline()
                if line:
                    self.lines.append(line)
                    self.line_func(line)
                else:
                    return

        self.thread = threading.Thread(target = collect_lines)
        self.thread.daemon = True
        self.thread.start()

    def get_output(self):
        return "".join(self.lines)

def pytest_itemcollected(item):
    """Outputs test class and function docstrings, if provided, when each test is run."""

    par = item.parent.obj
    node = item.obj
    pref = par.__doc__.strip() if par.__doc__ else par.__class__.__name__
    suf = node.__doc__.strip() if node.__doc__ else node.__name__
    if pref or suf:
        item._nodeid = ": Sipi should ".join((pref, suf))
