import subprocess
from subprocess import Popen, PIPE
import shlex
import os

# d = Popen(['ls', '-l'], stdout=PIPE, text=True)
# for line in d.stdout:
#     print(line)

# sys.exit(0)

os.environ['PATH'] = os.path.dirname(os.path.realpath(__file__)) + \
    '/../build:' + os.environ['PATH']

##################
# Helper functions
##################


class Daemon(object):
    def __init__(self, config, await_listening=True, assert_returncode=True):
        with open('config.toml', 'w') as cfg:
            cfg.write(config)
        self.process = Popen(['boardproxy'] +
                             ['--daemon', '--config=config.toml', '.'],
                             stdout=PIPE, stderr=PIPE, text=True)
        self.assert_returncode = assert_returncode
        if await_listening:
            # Wait until the daemon is able to accept connections
            for line in self.process.stderr:
                if "Listening in" in line:
                    break
                print(line)

    def __enter__(self):
        return self.process

    def __exit__(self, type, value, traceback):
        self.process.terminate()
        self.process.wait()
        if self.assert_returncode:
            assert self.process.returncode == 0


def run(args):
    if type(args) == str:
        args = shlex.split(args)
    return subprocess.run(args, capture_output=True, text=True)


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
'''
    with Daemon(config, await_listening=False, assert_returncode=False) as daemon:
        (stdout, stderr) = daemon.communicate()
        assert daemon.returncode == 1
        assert "no or invalid ip_address" in stderr


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
command = "echo This is board 1"
ip_address = "127.0.0.1"
'''
    with Daemon(config):
        client = Popen(['boardproxy', '.'], stdout=PIPE, text=True)
        board_ok = False
        for line in client.stdout:
            if line == "This is board 1\n":
                board_ok = True
                break
            print(line)
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

        for c in [client1, client2, client3]:
            c.terminate()
            c.wait()
