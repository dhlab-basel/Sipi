import pytest
import configparser
import os
import os.path
import shlex, signal
from subprocess import Popen, PIPE, STDOUT
from threading import Thread


@pytest.fixture
def manager(scope='module'):
    manager = SipiTestManager()
    manager.start_sipi()
    yield manager
    manager.stop_sipi()


class SipiTestManager:
    '''Controls Sipi and Nginx during tests.'''

    def __init__(self):
        self.config = configparser.ConfigParser()
        self.config.read('config.ini')

        self.sipi_working_dir = os.path.abspath('..')
        sipi_config = self.config['Sipi']
        self.sipi_config_file = sipi_config['config-file']
        self.sipi_command = "local/bin/sipi -config config/{}".format(self.sipi_config_file)
        self.data_dir = os.path.join(self.sipi_working_dir, self.config['Test']['data-dir'])
        self.sipi_base_url = sipi_config['base-url']
        self.sipi_ready_output = sipi_config['ready-output']
        self.sipi_start_wait = int(sipi_config['start-wait'])
        self.sipi_stop_wait = int(sipi_config['stop-wait'])
        self.sipi_process = None

        self.nginx_base_url = self.config['Nginx']['base-url']
        self.nginx_working_dir = os.path.abspath('nginx')
        self.start_nginx_command = 'nginx -p {} -c nginx.conf'.format(self.nginx_working_dir)
        self.stop_nginx_command = 'nginx -p {} -c nginx.conf -s stop'.format(self.nginx_working_dir)


    def start_sipi(self):
        sipi_args = shlex.split(self.sipi_command)
        self.sipi_process = Popen(sipi_args, cwd=self.sipi_working_dir, stdout=PIPE, stderr=STDOUT, universal_newlines=True)
        self.sipi_output_reader = ProcessOutputReader(self.sipi_process.stdout)

    def stop_sipi(self):
        if (self.sipi_process != None):
            self.sipi_process.send_signal(signal.SIGINT)
            self.sipi_process.wait(timeout=self.sipi_stop_wait)
            self.sipi_process = None

    def sipi_is_running(self):
        if (self.sipi_process == None):
            return false
        else:
            return self.sipi_process.poll() == None

    def get_sipi_output(self):
        self.sipi_output_reader.get_output()


class ProcessOutputReader:
    '''Spawns a thread that collects the output of a subprocess.'''

    def __init__(self, stream):
        self.stream = stream
        self.lines = []

        def collect_lines(stream, lines):
            while True:
                line = stream.readline()
                if line:
                    lines.append(line)
                else:
                    return

        self.thread = Thread(target = collect_lines, args = (self.stream, self.lines))
        self.thread.daemon = True
        self.thread.start()

    def get_output(self):
        '\n'.join(self.lines)
