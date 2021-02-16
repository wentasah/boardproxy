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
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <system_error>
#include <algorithm>
#include <sstream>
#include <functional>
#include <unistd.h>
#include "session.hpp"

#include "tcpproxy.hpp"

using namespace std;

TcpProxy::TcpProxy(Session &session, std::unique_ptr<UnixSocket> client_sock, uint16_t port)
    : SocketProxy("tcpproxy", session, move(client_sock))
    , target(client->watcher.loop)
{
    buf_c2t.reserve(65536);
    buf_t2c.reserve(65536);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    auto &ip = session.get_board()->ip_address;
    int ret = inet_aton(ip.c_str(), &addr.sin_addr);
    if (ret == 0)
        throw runtime_error(fmt::format("Invalid board IP address: {}", ip));

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1)
        throw std::system_error(errno, std::generic_category(), "socket(SOCK_STREAM)");

    info("Connecting to {}:{}", ip, port);

    ret = ::connect(sock, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1 && errno == EINPROGRESS) {
        target.set(sock, ev::WRITE);
        target.set<TcpProxy, &TcpProxy::on_target_connected>(this);
        target.start();
    } else if (ret == -1) {
        ::close(sock);
        throw system_error(errno, std::generic_category(), fmt::format("connect {}:{}", ip, port));
    } else {
        target.set(sock, ev::NONE);
        on_target_connected(target, ev::WRITE);
    }
}

TcpProxy::~TcpProxy()
{
    info("End of connection");
    target.stop();
    ::close(target.fd);
}

void TcpProxy::on_target_connected(ev::io &w, int revents)
{
    int so_err;
    socklen_t so_err_len = sizeof(so_err);
    int ret = ::getsockopt(w.fd, SOL_SOCKET, SO_ERROR, &so_err, &so_err_len);
    if (ret == -1)
        return fail("getsockopt(SO_ERROR): ", strerror(errno));
    if (so_err != 0)
        return fail("connect failure: {}", strerror(so_err));

    using namespace std::placeholders;  // for _1, _2, _3...

    target.set(ev::READ);
    target.set<TcpProxy, &TcpProxy::on_target_data>(this);
    target.start();

    client->watcher.set<TcpProxy, &TcpProxy::on_client_data>(this);
    client->watcher.start();
}

void TcpProxy::on_data(ev::io &from, ev::io &to, buffer_t &buf, int revents, const string_view from_name, const string_view to_name)
{
    int ret = ::read(from.fd, buf.data() + buf.size(), buf.capacity() - buf.size());
    if (ret == -1)
        return fail("{} read error: {}", from_name, strerror(errno));
    else if (ret == 0) {
        info("{} closed the connection", from_name);
        return close();
    }
    buf.resize(buf.size() + ret);

    logger->trace("{}->{} {}B", from_name, to_name, buf.size());
    ret = ::send(to.fd, buf.data(), buf.size(), 0);
    if (ret == -1)
        return fail("{} send error: {}", to_name, strerror(errno));
    // TODO: Handle short write by throttling the sender
    buf.erase(buf.begin(), buf.begin() + ret);
}
