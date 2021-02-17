# boardproxy

Manage remote access to a pool of (embedded) boards or computers and
allow SSH-based port forwarding.

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Features](#features)
- [Compilation](#compilation)
- [Usage](#usage)
    - [Quick start](#quick-start)
    - [Port forwarding](#port-forwarding)
    - [SSH ForcedCommand](#ssh-forcedcommand)
    - [systemd socket activation](#systemd-socket-activation)
    - [Command templates](#command-templates)
    - [User management](#user-management)

<!-- markdown-toc end -->


# Features

- Automates board allocation to users and implements a wait queue.
- Typically configured to provide access to board's serial line via
  SSH.
- When used with SSH port forwarding, the port is automatically
  forwarded to the used board.
- Can execute a cleanup command when a user disconnects.

# Compilation

1. Install prerequisites (meson, ninja, libev, boost, libsystemd).

   Under Debian:

        apt install meson ninja-build libev-dev libboost-program-options-dev libsystemd-dev
2. Run

        make
		make install

# Usage

To access the board via boardproxy, prepare a [TOML](https://toml.io/) *configuration file*
that defines your boards and run the *boardproxy daemon*. Then use the
*boardproxy in client mode* to connect to the board.

## Quick start

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
runs the boardproxy client. The `ip_address` is used for port
forwarding.

The daemon is run as:

    boardproxy --daemon --config=config.toml

To connect one of the boards, run:

    boardproxy /run/my_boards

If a board is available, the daemon runs the corresponding `command`
with its stdin/out/err connected stdin/out/err of the client. The
commands above allow the user to interact with the board serial line
via the [sterm](https://github.com/wentasah/sterm) program.

If all boards are occupied, a list of boards and their users is
printed, and the new user waits. Whenever an existing user
disconnects, the first waiting user gets the board.

## Port forwarding

Boardproxy can forward network traffic to connected boards. A nice
feature is that users do not have to care about which board they
connect to – boardproxy ensures that the traffic is forwarded to the
board assigned to the user.

For example, to access the board's SSH and HTTP server, add the
following to the daemon configuration file:

``` toml
[sockets]
ssh = { type = "tcp", port = 22 }
www = { type = "tcp", port = 80 }
```

Then, users can use SSH-provided TCP-to-UNIX port forwarding to
forward TCP connections to `boardproxy` and boardproxy then proxies the
connection to the correct board. More specifically, when users connect
to the board like this:

    ssh -L2222:/run/my_boards/ssh -L8080:/run/my_boards/www login@board-server.example.com

then the board's SSH server can be reached with:

    ssh localhost:2222

and board's HTTP server is available as:

    http://localhost:8080

To make connecting the board easier, one can put the following to
their `~/.ssh/config`:

    Host my_board
         User login
         Hostname board-server.example.com
         LocalForward 2222 /run/my_boards/ssh
         LocalForward 8080  /run/my_boards/www
         ExitOnForwardFailure yes

## SSH ForcedCommand

If you don't want to give your users shell access on the boardproxy
server, you can use SSH ForcedCommand to allow users to execute just
the boardproxy command and nothing more. Typical configuration in
`/etc/ssh/sshd_config` could look like this:

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

With this configuration, users (students) run the following command to
connect to the board:

    ssh login@board-server.example.com

The `--allow-set-authorized-keys` option allows users to set their SSH
`authorized_keys` via command like:

    ssh login@board-server.example.com set-authorized-keys < ~/.ssh/id_rsa.pub

## systemd socket activation

Boardproxy supports systemd socket activation. In this mode,
boardproxy is only started when a user is connected. When the last
user disconnects, boardproxy is stopped. Stopping the daemon is useful
for automatically re-reading configuration file after its change (note
that reloading the config without disconnecting currently connected
users is not possible).

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

For example, add the following to the spawned editor to allow all
members of students group to access your boards:

    [Socket]
    SocketGroup=students

## Command templates

Commands to execute by the daemon can be specified via a template.
There is no need to repeat long commands for each board. The template
(e.g. `command_template` or `close_command_template`) can contain
*replacement fields* in [fmt
syntax](https://fmt.dev/latest/syntax.html), which get replaced by
values specific for each board. See [this configuration
file](./configs/psr-hw.toml) for examples.

## User management

There are at least two ways, how boardproxy users can be managed:

- Each user has a separate UNIX account on the boardproxy server.

  In this case, users can run boardproxy clients manually or their
  accounts can be configured as described in [SSH
  ForcedCommand](#ssh-forcedcommand) section.

- All users share a single UNIX account and are authenticated by their
  SSH keys. In the `authorized_keys` file, the `command` option is
  used to run boardproxy. For example:

        command="boardproxy --name=username /run/my_boards" ssh-rsa ...



<!--  LocalWords:  boardproxy
 -->

<!-- Local Variables: -->
<!-- markdown-toc-user-toc-structure-manipulation-fn: cdr -->
<!-- End: -->
