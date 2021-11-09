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
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ev++.h>
#include <array>
#include "unix_socket.hpp"
#include "protocol.hpp"

// Client-side process (intended to be executed via SSH's ForceCommand)

class Client
{
public:
    Client(ev::loop_ref loop, std::string sock_dir,
           proto::setup::command_t command,
           std::string username, bool no_wait);
    ~Client();

private:
    std::array<char, 65536> buffer;
    UnixSocket socket;

    const bool is_connect;

    void on_data_from_daemon(ev::io &w, int revents);

    void send_setup(proto::setup::command_t command,
                    const std::string &username, bool no_wait);
};

#endif // CLIENT_HPP
