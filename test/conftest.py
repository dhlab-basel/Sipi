import pytest
import configparser
import os
import shlex, signal
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
        self.config.read("config.ini")

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

        # Wait until Sipi says it"s ready to receive requests.
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

    def download_file(self, url, headers=None):
        """
            Makes an HTTP request and downloads the response content to a temporary file.
            Returns a NamedTemporaryFile object that has been written to and closed.
        """

        response = requests.get(url, headers=headers, stream=True)
        response.raise_for_status()
        temp_file = tempfile.NamedTemporaryFile(delete=False)

        for chunk in response.iter_content(chunk_size=8192):
            temp_file.write(chunk)

        temp_file.close()
        return temp_file

    def compare(self, url_path, filename, headers=None):
        """
            Downloads a file and compares it with another file on disk.

            url_path: a path that will be appended to the Sipi base URL to make the request.
            filename: the name of a file in the test data directory, containing the expected data.
            headers: an optional dictionary of request headers.

            Returns True if the files contain the same data.
        """

        original_file_path = os.path.join(self.data_dir, filename)
        url = "{}/{}".format(self.sipi_base_url, url_path)
        downloaded_file = self.download_file(url, headers)
        return filecmp.cmp(original_file_path, downloaded_file.name)


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
