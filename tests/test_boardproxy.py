import subprocess
from subprocess import Popen, PIPE
import shlex
import os
from datetime import datetime
import time
import pytest

# d = Popen(['ls', '-l'], stdout=PIPE, text=True)
# for line in d.stdout:
#     print(line)

# sys.exit(0)

os.environ['PATH'] = os.path.dirname(os.path.realpath(__file__)) + \
    '/../build/src:' + os.environ['PATH']

##################
# Helper functions
##################


def read_lines_until(text, io, print_prefix=""):
    for line in io:
        if print_prefix:
            print(print_prefix, line.rstrip())
        if text in line:
            break


class Daemon(object):
    def __init__(self, config,
                 await_listening=True,
                 assert_returncode=True,
                 systemd_socket_activate=False,
                 terminate=True):
        self.assert_returncode = assert_returncode
        self.systemd_socket_activate = systemd_socket_activate
        self.terminate = terminate

        with open('config.toml', 'w') as cfg:
            cfg.write(config)
        cmd_prefix = []
        # cmd_prefix = ['strace', '-f', '-DD', '--timestamps=time,ms', '-o', 'strace.log']

        if systemd_socket_activate:
            cmd_prefix = ['systemd-socket-activate', '--seqpacket',
                          '--listen={}/boardproxy'.format(os.getcwd())]
        self.process = Popen(cmd_prefix + ['boardproxy'] +
                             ['--daemon', '--config=config.toml', '.'],
                             stdout=PIPE, stderr=PIPE, text=True)
        if await_listening:
            # Wait until the daemon is able to accept connections
            for line in self.process.stderr:
                print("Daemon stderr:", line.rstrip())
                if "Listening" in line:
                    # This matches output of both daemon and
                    # systemd-socket-activate
                    break

    def __enter__(self):
        return self.process

    def __exit__(self, type, value, traceback):
        # Under socket activation, the daemon should terminate
        # automatically, we terminate it manually otherwise
        if not self.systemd_socket_activate and self.terminate:
            self.process.terminate()
        self.process.wait()

        # Save stdout/err for later processing (if needed)
        self.stdout = ''
        for line in self.process.stdout:
            self.stdout += line
            print("Daemon stdout:", line.rstrip())
        self.stderr = ''
        for line in self.process.stderr:
            self.stderr += line
            print("Daemon stderr:", line.rstrip())

        if self.assert_returncode:
            assert self.process.returncode == 0


def run(args, **kwargs):
    if type(args) == str and kwargs.get("shell", False) == False:
        args = shlex.split(args)
    res = subprocess.run(args, capture_output=True, text=True, **kwargs)
    print("stdout of {}:".format(shlex.join(args)), res.stdout)
    print("stderr of {}:".format(shlex.join(args)), res.stderr)
    return res


@pytest.fixture(autouse=True)
def run_in_tmp_path(tmp_path):
    print("Changing directory to", tmp_path)
    os.chdir(tmp_path)


#######
# Tests
#######

def test_no_sock_dir():
    res = run('boardproxy --daemon')
    assert res.returncode != 0
    assert "sock_dir not specified" in res.stderr


def test_missing_ip_address():
    config = '''\
[boards.1]
command = "echo This is board 1"
# ip_address missing
'''
    daemon = Daemon(config, await_listening=False, assert_returncode=False, terminate=False)
    with daemon:
        pass
    assert daemon.process.returncode == 1
    assert "no or invalid ip_address" in daemon.stderr


def test_simple_command():
    config = '''\
[boards.1]
command = "echo This is board 1"
ip_address = "127.0.0.1"
'''
    with Daemon(config):
        res = run('boardproxy .')
        assert res.returncode == 0
        assert "This is board 1" in res.stdout


def test_disconnect():
    config = '''\
[boards.1]
command = "bash -c 'echo This is board 1; read -t2; echo Done'"
ip_address = "127.0.0.1"
'''
    with Daemon(config):
        client = Popen(['boardproxy', '.'], stdout=PIPE, text=True)
        board_ok = False
        for line in client.stdout:
            print(line.rstrip())
            if line == "This is board 1\n":
                board_ok = True
                break
        client.terminate()  # Disconnect after first line
        client.wait()
        assert client.returncode == -15
        assert board_ok
        assert "Done" not in client.stdout


def test_command_template():
    config = '''\
command_template = "echo This is board {name}"
[boards.1]
name = "ABC"
ip_address = "127.0.0.1"
'''
    with Daemon(config):
        res = run('boardproxy .')
        assert res.returncode == 0
        assert "This is board ABC" in res.stdout


def test_two_boards_three_clients():
    config = '''\
command_template = "bash -c 'echo This is board {name}; sleep inf'"
[boards]
board1 = { name = "A", ip_address = "127.0.0.1" }
board2 = { name = "B", ip_address = "127.0.0.1" }
'''
    with Daemon(config):
        client1 = Popen(['boardproxy', '.'], stdout=PIPE, stderr=PIPE, text=True)
        # Reading below also ensures that next client is started after
        # the first is connected
        assert client1.stderr.readline() == "Connecting to board board1\n"
        assert client1.stdout.readline() == "This is board A\n"

        client2 = Popen(['boardproxy', '.'], stdout=PIPE, stderr=PIPE, text=True)
        assert client2.stderr.readline() == "Connecting to board board2\n"
        assert client2.stdout.readline() == "This is board B\n"

        client3 = Popen(['boardproxy', '.'], stdout=PIPE, stderr=PIPE, text=True)
        assert "board1" in client3.stderr.readline()
        assert "board2" in client3.stderr.readline()
        assert "waiting" in client3.stderr.readline()
        assert client3.stderr.readline() == "No board currently available. Waiting...\n"

        for client in [client1, client2, client3]:
            client.terminate()
            client.wait()


def test_systemd_socket_activation():
    config = '''\
[boards.1]
command = "echo This is board 1"
ip_address = "127.0.0.1"
'''
    with Daemon(config, systemd_socket_activate=True):
        res = run('boardproxy .')
        assert res.returncode == 0
        assert "This is board 1" in res.stdout


# TODO: close_command test


def test_tcp_proxy():
    config = '''\
[sockets]
hello = { type = "tcp", port = 1234 }

[boards.1]
command = "bash -c 'echo This is board 1; read'"
ip_address = "127.0.0.1"'''
    # with Popen('echo Hello | nc -l -N 1234', shell=True) as server:
    with Popen('systemd-socket-activate --listen=1234 --accept --inetd echo hello',
               shell=True, stderr=PIPE, text=True) as server:
        read_lines_until("Listening", server.stderr)
        with Daemon(config):
            with Popen('ssh -tt -L4321:{sock_dir}/hello localhost boardproxy {sock_dir}'.format(sock_dir=os.getcwd()),
                       shell=True, stdout=PIPE, text=True) as client:
                read_lines_until("Connecting to board", client.stdout, print_prefix="Client stdout:")
                res = run('nc localhost 1234 < /dev/null', shell=True)
                client.terminate()
                server.terminate()
                assert res.returncode == 0
                assert "hello" in res.stdout


def test_multipe_socket_proxies():
    config = '''\
[sockets]
ssh = { type = "tcp", port = 22 }
www = { type = "tcp", port = 80 }
wrproxy = { type = "wrproxy" }

[boards.1]
command = "bash -c 'echo This is board 1; read'"
ip_address = "127.0.0.1"'''
    with Daemon(config):
        assert os.path.exists('ssh')
        assert os.path.exists('www')
        assert os.path.exists('wrproxy')


def test_no_wait():
    config = '''\
[boards.1]
command = "bash -c 'echo This is board 1; sleep inf'"
ip_address = "127.0.0.1"
'''
    with Daemon(config):
        # Occupy the board with a client
        client = Popen(['boardproxy', '.'], stdout=PIPE, text=True)
        for line in client.stdout:
            print(line.rstrip())
            if line == "This is board 1\n":
                break
        # Try connecting another client without waiting
        res = run('boardproxy . --no-wait') # Should exit immediately
        assert "No board currently available.\n" in res.stderr # No "Waiting..." is present
        client.terminate() # terminate the first client
        client.wait()
