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
#include <unistd.h>
#include "session.hpp"

#include "wrproxy.hpp"

using namespace std;

WrProxy::WrProxy(Session &session, std::unique_ptr<UnixSocket> client_sock)
    : SocketProxy("wrproxy", session, move(client_sock))
{
    buf_c2t.reserve(65536);

    client->watcher.set<WrProxy, &WrProxy::on_client_data>(this);
    client->watcher.start();

    int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1)
        throw std::system_error(errno, std::generic_category(), "socket(SOCK_DGRAM)");
    target.set(sock, ev::READ);
    target.set<WrProxy, &WrProxy::on_target_data>(this);
    target.start();

    info("New connection");
}

WrProxy::~WrProxy()
{
    info("End of connection");
    target.stop();
    ::close(target.fd);
}

void WrProxy::on_client_data(ev::io &w, int revents)
{
    auto &buf = buf_c2t;

    int ret = ::read(w.fd, buf.data() + buf.size(), buf.capacity() - buf.size());
    if (ret == -1) {
        logger->error("wrproxy: client read error: {}", strerror(errno));
        return close();
    } else if (ret == 0) {
        info("client closed the connection");
        return close();
    }
    buf.resize(buf.size() + ret);

    switch (state) {
    case COMMAND:
        return parse_command();
    case DATA: {
        logger->trace("wrproxy: DATA {}", buf.size());
        uint32_t len;
        if (buf.size() < sizeof(len))
            break;
        len = be32toh(*(uint32_t*)buf.data());
        if (buf.size() < sizeof(len) + len)
            break;
        ret = ::write(target.fd, buf.data() + sizeof(len), len);
        if (ret == -1) {
            // TODO: Handle full buffers
            logger->error("wrproxy: Target sock write error: {}", strerror(errno));
            return close();
        }
        buf.erase(buf.begin(), buf.begin() + sizeof(len) + len);
        break;
    }
    }

}

void WrProxy::on_target_data(ev::io &w, int revents)
{
    auto &buf = buf_t2c;

    int ret = ::read(w.fd, buf.data(), buf.size());
    if (ret == -1) {
        logger->error("wrproxy: target read error: {}", strerror(errno));
        return close();
    } else if (ret == 0) {
        info("target closed the connection???");
        return close();
    }
    logger->trace("wrproxy: target sent {} bytes", ret);

    size_t size = ret;
    uint32_t len = htobe32(size);
    // TODO: Handle full buffers and other errors
    ret = ::write(client->watcher.fd, &len, sizeof(len));
    ret = ::write(client->watcher.fd, &buf, size);
    if (ret == -1) {
        logger->error("wrproxy: Write to client failed: {}", strerror(errno));
        return close();
    }
}

void WrProxy::parse_command()
{
    string buf(begin(buf_c2t), end(buf_c2t));
    auto last = buf.find_first_of("\0\n\r"s);

    if (last == string::npos)
        return;

    buf.resize(last);
    info("Received command: {}", buf);

    istringstream cmd(buf);
    string command;
    std::getline(cmd, command, ' ');
    while (cmd.peek() == ' ')
        cmd.ignore(1);
    if (command == "connect") {
        // immediately return, because handle_connect could have called close()
        return handle_connect(cmd);
    } else {
        return fail(fmt::format("Unsupported command: {}", command));
    }
}

void WrProxy::handle_connect(istringstream &cmd)
{
    // The following command is sent by WindRiver Workbench:
    // connect type=udpsock;tgt=10.35.95.50;port=17185;mode=packet;mode=packet

    string type, tgt, port, mode;

    while (!cmd.eof()) {
        string param;
        std::getline(cmd, param, ';');
        auto pos = param.find('=');
        if (pos == string::npos)
            return fail("Missing '=' in connect parameter: {}", param);

        auto key = param.substr(0, pos);
        auto val = param.substr(pos + 1);
        if (key == "type")
            type = val;
        else if (key == "tgt")
            tgt = val;
        else if (key == "port")
            port = val;
        else if (key == "mode")
            mode = val;
        else
            return fail("Unsupported parameter: {}", param);
    }

    if (type == "udpsock" && mode == "packet") {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        try {
            addr.sin_port = htons(stoi(port));
        } catch (...) {
            return fail("Bad port {}", port);
        }
        if (!session.get_board())
            return fail("No board assigned");

        auto &ip = session.get_board()->ip_address;
        int ret = inet_aton(ip.c_str(), &addr.sin_addr);
        if (ret == 0)
            return fail("Invalid board IP address: {}", ip);

        ret = ::connect(target.fd, (sockaddr*)&addr, sizeof(addr));
        if (ret == -1)
            return fail("connect {}:{}: {}", ip, port, strerror(errno));

        info("connected to {}:{}", ip, port);
        state = DATA;    // Switch to data mode - commands will no longer be accepted
        buf_c2t.clear(); // clear the rest of EOL characters (e.g. \r\n etc.)
        ret = ::write(client->watcher.fd, "ok\0", 3);
        if (ret == -1)
            return fail("Writing ok response failed: {}", strerror(errno));
    } else {
        return fail("Unsupported type ({}) or mode ({})", type, mode);
    }
}

template<typename FormatString, typename... Args>
void WrProxy::fail(const FormatString &fmt, const Args &... args)
{
    // First send an error message to wrpoxy client, e.g., WindRiver Workbench
    auto msg = fmt::format(fmt, args...);
    string resp = fmt::format("error wrproxy: {}", msg);
    int ret = ::write(client->watcher.fd, resp.c_str(), resp.size() + 1 /* zero delimitter */);
    if (ret == -1)
        logger->error("wrproxy: fail: write error: {}", strerror(errno));

    // Then log the error and close the proxy
    return SocketProxy::fail(msg);
}
