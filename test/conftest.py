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
from threading import Thread
import time
import requests
import tempfile
import filecmp

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
        sipi_config = self.config["Sipi"]
        self.sipi_config_file = sipi_config["config-file"]
        self.sipi_command = "local/bin/sipi -config config/{}".format(self.sipi_config_file)
        self.data_dir = os.path.abspath(self.config["Test"]["data-dir"])
        self.sipi_base_url = sipi_config["base-url"]
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

    def start_sipi(self):
        """Starts Sipi and waits until it is ready to receive requests."""

        def check_for_ready_output(line):
            if self.sipi_ready_output in line:
                self.sipi_started = True

        # Start a Sipi process and capture its output.
        sipi_args = shlex.split(self.sipi_command)
        sipi_start_time = time.time()
        self.sipi_process = subprocess.Popen(sipi_args, cwd=self.sipi_working_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        self.sipi_output_reader = ProcessOutputReader(self.sipi_process.stdout, check_for_ready_output)

        # Wait until Sipi says it's ready to receive requests.
        while (not self.sipi_started) and (not self.sipi_took_too_long):
            time.sleep(0.2)
            if (time.time() - sipi_start_time > self.sipi_start_wait):
                self.sipi_took_too_long = True

        assert not self.sipi_took_too_long, "Sipi didn't start after {} seconds".format(self.sipi_start_wait)

    def stop_sipi(self):
        """Sends SIGINT to Sipi and waits for it to stop."""

        if (self.sipi_process != None):
            self.sipi_process.send_signal(signal.SIGINT)
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

        nginx_args = shlex.split(self.start_nginx_command)
        assert subprocess.run(nginx_args).returncode == 0, "nginx failed to start"

    def stop_nginx(self):
        """Stops nginx."""
        
        nginx_args = shlex.split(self.stop_nginx_command)
        assert subprocess.run(nginx_args).returncode == 0, "nginx failed to stop"        

    def download_file(self, url, headers=None, suffix=None):
        """
            Makes an HTTP request and downloads the response content to a temporary file.
            Returns the absolute path of the temporary file.
        """

        response = requests.get(url, headers=headers, stream=True)
        response.raise_for_status()
        temp_fd, temp_file_path = tempfile.mkstemp(suffix=suffix)
        temp_file = os.fdopen(temp_fd, mode="wb")

        for chunk in response.iter_content(chunk_size=8192):
            temp_file.write(chunk)

        temp_file.close()
        return temp_file_path

    def compare(self, url_path, filename, headers=None, just_size=False):
        """
            Downloads a file and compares it with a temporary file on disk. If the two are equivalent,
            deletes the temporary file, otherwise throws an exception.

            url_path: a path that will be appended to the Sipi base URL to make the request.
            filename: the name of a file in the test data directory, containing the expected data.
            headers: an optional dictionary of request headers.
            just_size: if True, just checks that the files have the same number of bytes.
        """

        expected_file_path = os.path.join(self.data_dir, filename)
        expected_file_basename, expected_file_extension = os.path.splitext(expected_file_path)
        url = "{}/{}".format(self.sipi_base_url, url_path)
        downloaded_file_path = self.download_file(url, headers=headers, suffix=expected_file_extension)

        if just_size:
            downloaded_file_size = os.stat(downloaded_file_path).st_size
            expected_file_size = os.stat(expected_file_path).st_size

            if downloaded_file_size == expected_file_size:
                os.remove(downloaded_file_path)
            else:
                assert False, "The size of the downloaded file {} ({} bytes) does not equal the size of the expected file {} ({} bytes)".format(downloaded_file_path, downloaded_file_size, expected_file_path, expected_file_size)
        else:
            if filecmp.cmp(downloaded_file_path, expected_file_path):
                os.remove(downloaded_file_path)
            else:
                assert False, "Downloaded file {} is different from expected file {}".format(downloaded_file_path, expected_file_path)


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

        self.thread = Thread(target = collect_lines)
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
