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
#ifndef UNIX_SOCK_HPP
#define UNIX_SOCK_HPP

#include <string>
#include <ev++.h>
#include <memory>

class UnixSocket
{
public:
    enum class type { seqpacket, stream, datagram };

    UnixSocket(ev::loop_ref loop, type t, bool allow_socket_activation = false);
    ~UnixSocket();

    void connect(std::string path);
    void bind(std::string path);
    void listen();

    std::unique_ptr<UnixSocket> accept();

    void make_nonblocking();
    void close();

    struct ucred peer_cred();

    ev::io watcher;
    const bool is_from_systemd;
private:
    UnixSocket(ev::loop_ref loop, int fd);
};

#endif // UNIX_SOCK_HPP
