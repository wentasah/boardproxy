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
#include <systemd/sd-daemon.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <system_error>
#include <err.h>
#include "unix_socket.hpp"

UnixSocket::UnixSocket(ev::loop_ref loop, type t, bool allow_socket_activation)
    : watcher(loop)
    , is_from_systemd(allow_socket_activation && sd_listen_fds(1) > 0)
{
    int sock_type = SOCK_SEQPACKET;

    switch (t) {
    case type::datagram:  sock_type = SOCK_DGRAM; break;
    case type::seqpacket: sock_type = SOCK_SEQPACKET; break;
    case type::stream:    sock_type = SOCK_STREAM; break;
    }

    int fd;
    if (is_from_systemd) {
        fd = SD_LISTEN_FDS_START;
        int ret = sd_is_socket(fd, AF_UNIX, sock_type, 1);
        if (ret < 0)
            throw std::system_error(-ret, std::generic_category(), "sd_is_socket");
        if (ret == 0)
            throw std::runtime_error("Invalid socket from systemd");
    } else {
        fd = socket(AF_UNIX, sock_type | SOCK_CLOEXEC, 0);
        if (fd == -1)
                throw std::system_error(errno, std::generic_category(), "socket(AF_UNIX)");
    }

    watcher.set(fd, ev::READ);
}

UnixSocket::UnixSocket(ev::loop_ref loop, int fd)
    : watcher(loop)
    , is_from_systemd(false)
{
    watcher.set(fd, ev::READ);
}


UnixSocket::~UnixSocket()
{
    if (watcher.is_active())
        watcher.stop();
    close();
}

void UnixSocket::connect(std::string path)
{
    struct sockaddr_un name;

    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, path.c_str(), sizeof(name.sun_path) - 1);

    int ret = ::connect(watcher.fd, reinterpret_cast<sockaddr*>(&name), sizeof(name));
    if (ret == -1)
        throw std::system_error(errno, std::generic_category(), path);

    make_nonblocking();
}

void UnixSocket::bind(std::string path)
{
    struct sockaddr_un name;

    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, path.c_str(), sizeof(name.sun_path) - 1);

    int ret = ::bind(watcher.fd, reinterpret_cast<sockaddr*>(&name), sizeof(name));
    if (ret == -1)
        throw std::system_error(errno, std::generic_category(), path);

    make_nonblocking();
}

void UnixSocket::listen()
{
    int ret = ::listen(watcher.fd, 10);
    if (ret == -1 && errno != EAGAIN)
        throw std::system_error(errno, std::generic_category(), "listen");
}

std::unique_ptr<UnixSocket> UnixSocket::accept()
{
    int ret = ::accept4(watcher.fd, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (ret == -1)
        throw std::system_error(errno, std::generic_category(), "accept");
    return std::unique_ptr<UnixSocket>(new UnixSocket(watcher.loop, ret));
}

void UnixSocket::make_nonblocking()
{
    int flags = fcntl(watcher.fd, F_GETFL);
    if (flags == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL)");
    int ret = fcntl(watcher.fd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL, O_NONBLOCK)");
}

void UnixSocket::close()
{
    if (watcher.fd != -1)
        ::close(watcher.fd);
    watcher.fd = -1;
}

struct ucred UnixSocket::peer_cred()
{
    struct ucred cred;
    socklen_t len = sizeof(cred);
    int ret = getsockopt(watcher.fd, SOL_SOCKET, SO_PEERCRED, &cred, &len);
    if (ret == -1)
        throw std::system_error(errno, std::generic_category(), "getsockopt(SO_PEERCRED)");
    return cred;
}
