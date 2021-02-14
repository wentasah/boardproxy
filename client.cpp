// Copyright (C) 2021 Michal Sojka <michal.sojka@cvut.cz>
// 
// This file is part of boardproxy.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <functional>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include "client.hpp"
#include "unix_socket.hpp"
#include "protocol.hpp"

using namespace std;

Client::Client(ev::loop_ref loop, std::string sock_dir,
               proto::setup::command_t command,
               std::string username)
    : socket(loop, UnixSocket::type::seqpacket)
    , is_connect(command == proto::setup::command::connect)
{
    if (is_connect && isatty(STDOUT_FILENO))
        std::cout << "Welcome to boardproxy" << std::endl;
    socket.connect(sock_dir + "/boardproxy");
    socket.watcher.set<Client, &Client::on_data_from_daemon>(this);
    socket.watcher.start();

    send_setup(command, username);
}

Client::~Client()
{
}

void Client::on_data_from_daemon(ev::io &w, int revents)
{
    ssize_t ret = ::read(w.fd, &buffer, sizeof(buffer));
    if (ret == 0) {
        if (is_connect)
            cerr << "Server closed connection" << endl;
        w.stop();
    }
}

void Client::send_setup(proto::setup::command_t command, const std::string &username)
{
    // Send stdin/out/err to the daemon so that it can run the proxied
    // process on them
    int myfds[3] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
    // Also send addition data, needed by the daemon
    proto::setup data(command, getppid(), username);

    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg;
    struct iovec io = {
        .iov_base = &data,
        .iov_len = sizeof(data)
    };
    union {         /* Ancillary data buffer, wrapped in a union
                       in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(sizeof(myfds))];
        struct cmsghdr align;
    } u;

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(myfds));
    memcpy(CMSG_DATA(cmsg), myfds, sizeof(myfds));

    ::sendmsg(socket.watcher.fd, &msg, 0);

    close(STDIN_FILENO);
}
