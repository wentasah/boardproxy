# boardproxy

Manages remote access to a pool of (embedded) boards or computers.

# Features

- Automates board allocation to users and implements wait queue.
- Can provides access to board's serial line via SSH.
- When used with SSH port forwarding, the port is automatically
  forwarded to the used board.
- Can execute a cleanup command when user disconnects.

# Compilation

1. Install prerequisites (meson, ninja, libev, boost, libsystemd).

   Under Debian:

        apt install meson ninja-build libev-dev libboost-program-options-dev libsystemd-dev
2. Run

        make
		make install

# Usage

To access the board via boardproxy, prepare a [TOML](https://toml.io/)
configuration file that defines your boards and run boardproxy daemon.

A simple configuration file (`config.toml`) can look like this:

``` toml
sock_dir = "/run/my_boards"

[boards.1]
command = "sterm -s 115200 -d10 /dev/ttyUSB0"
ip_address = "192.168.1.2"

[boards.2]
command = "sterm -s 115200 -d10 /dev/ttyUSB1"
ip_address = "192.168.1.3"
```

This defines two boards. The `command` is executed whenever somebody
runs the boardproxy client. `ip_address` is used for port forwarding.

The daemon is run as:

    boardproxy --daemon --config=config.toml

To connect one of the boards, run:

    boardproxy /run/my_board

If a board is available, the corresponding `command` is run and the
user can interact with its serial line. If all boards are occupied,
the list of current board users is printed and the user wait. Whenever
an existing user disconnects, waiting user gets the access.

## SSH ForcedCommand

If you don't want to give your users shell access on your server, you
can use SSH ForcedCommand to allows users executing just boardproxy
and nothing more. Typical configuration in `/etc/ssh/sshd_config`
could look like this:

    Match Group students
      ForceCommand /usr/local/bin/boardproxy --allow-set-authorized-keys /run/my_boards
      AllowStreamLocalForwarding local # needed for
    #   AllowTcpForwarding no
      AllowTcpForwarding local 	# should be no, but does not work
      AllowAgentForwarding no
      PermitUserRC no
      X11Forwarding no
      ClientAliveInterval 60
      ClientAliveCountMax 5

With this configurations, users (students) run the following command
to connect to the baord:

    ssh login@board-server.example.com

The `--allow-set-authorized-keys` option allows users to set their SSH
`authorized_keys` via command like:

    ssh login@board-server.example.com set-authorized-keys < ~/.ssh/id_rsa.pub

## systemd socket activation

Boardproxy supports systemd socket activation. In this mode,
boardproxy is only started when a user is connected. When the last
user disconnects, boardproxy is stopped. This is useful for
automatically re-reading configuration file after its change.

We provide systemd template units `boardproxy@.socket` and
`boardproxy@.service`. Typically, administrators will need to override
certain settings in these units for proper operation on their systems.
The unit instance (text between `@` and `.`) determines the user under
which the daemon runs. Having several instances allows having multiple
independent board pools (e.g. boards of different type).

Enable socket activation by:

    systemctl enable boardproxy@my_boards.socket

And if needed, override its settings:

    systemctl edit boardproxy@my_boards.socket

For example, add the following to the spawned editor to allow every
student to access your boards:

    [Socket]
    SocketGroup=students

## Port forwarding

Boardproxy supports port forwarding. A nice feature is that users do
not have to care about which board they connect – boardproxy ensures
that ports get forwarded to their board.

To access the boards SSH and HTTP server, add the following the
configuration file:

``` toml
[sockets]
ssh = { type = "tcp", port = 22 }
www = { type = "tcp", port = 80 }
```

Then, users can users can use SSH-provided TCP-to-UNIX port forwarding
to forward TCP connections to `boardproxy`. Boardproxy then proxies
the connection to the correct board. More specifically, when users
connect to the board like this:

    ssh -L2222:/run/my_boards/ssh -L8080:/run/my_boards/www login@board-server.example.com

then board's SSH server can be reached with:

    ssh localhost:2222

and boards' HTTP server is available as:

    http://localhost:8080

To make connecting the board easier, one can put the following to
their `~/.ssh/config`:

    Host my_board
         User login
         Hostname board-server.example.com
         LocalForward 2222 /run/my_boards/ssh
         LocalForward 8080  /run/my_boards/www
         ExitOnForwardFailure yes


<!--  LocalWords:  boardproxy
 -->
