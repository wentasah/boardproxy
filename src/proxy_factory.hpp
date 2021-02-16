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
#ifndef PROXY_FACTORY_HPP
#define PROXY_FACTORY_HPP

#include <string>
#include <memory>
#include "socket_proxy.hpp"

class ProxyFactory {
public:
    const std::string sock_name;

    ProxyFactory(std::string sock_name) : sock_name(sock_name) {}
    virtual ~ProxyFactory() {}

    virtual std::unique_ptr<SocketProxy>
    create(Session &session, std::unique_ptr<UnixSocket> client_sock) = 0;
};

#endif // PROXY_FACTORY_HPP
