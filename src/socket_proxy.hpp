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
#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <memory>
#include <spdlog/logger.h>
#include "unix_socket.hpp"

class Session;

/// Abstract base class for proxied sockets
class SocketProxy
{
public:
    SocketProxy(const std::string proxy_type,
                Session &session,
                std::unique_ptr<UnixSocket> client_sock);
    virtual ~SocketProxy() {}
protected:
    const std::string proxy_type;
    Session &session;
    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<UnixSocket> client;
    const uint64_t id;

    template<typename FormatString, typename... Args>
    void info(const FormatString &fmt, const Args &... args) {
        logger->info("{} {}: {}", proxy_type, id, fmt::format(fmt, args...));
    }

    template<typename FormatString, typename... Args>
    void fail(const FormatString &fmt, const Args &... args) {
        logger->error("{} {}: {}", proxy_type, id, fmt::format(fmt, args...));
        return close();
    }

    void close();
};

#endif // SOCKET_HPP
