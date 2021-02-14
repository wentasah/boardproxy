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
#include "socket_proxy.hpp"
#include "session.hpp"

SocketProxy::SocketProxy(const std::string proxy_type, Session &session, std::unique_ptr<UnixSocket> client_sock)
    : proxy_type(proxy_type)
    , session(session)
    , logger(session.logger)
    , client(move(client_sock))
    , id(session.proxy_cnt++)
{
}

void SocketProxy::close()
{
    return session.proxies.remove_if([&](auto &proxy) { return proxy.get() == this; });
}
