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
#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <sys/types.h>
#include <string>
#include <cstring>

namespace proto {

enum class msg_type {
    setup
};

struct header {
    msg_type id;
    uint32_t length; // length without this header

    header() {}
    header(msg_type id, uint32_t length) : id(id), length(length) {}
};

// Setup message sent from client to daemon when the client starts. In
// addition to the data fields seen below, the client also sends with
// this message ancillary data with client's stdin/out/err file
// descriptors.
struct setup : header {
    using command_t = enum class command {
        connect,
        list_sessions,
    };

    command_t cmd;
    pid_t ppid;
    char username[32] = { 0 };

    setup(command_t command,
          pid_t ppid,
          const std::string user)
        : header(msg_type::setup, sizeof(*this))
        , cmd(command)
        , ppid(ppid)
    {
        strncpy(username, user.c_str(), sizeof(username) - 1);
    }
};

}
#endif
