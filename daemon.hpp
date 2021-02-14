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
#ifndef DAEMON_H
#define DAEMON_H

#include <list>
#include <string>
#include <ev++.h>
#include "session.hpp"
#include "unix_socket.hpp"
#include "board.hpp"
#include "proxy_factory.hpp"

class Daemon {
public:
    Daemon(ev::loop_ref &loop, std::string sock_dir,
           std::list<Board> boards,
           std::list<std::unique_ptr<ProxyFactory>> proxy_factories);
    ~Daemon();
    void run();
    void assign_board(Session *session);
    void close_session(Session *session);

    void print_status(int fd);
private:
    class ProxyListener {
    public:
        UnixSocket socket;
        std::unique_ptr<ProxyFactory> factory;

        ProxyListener(ev::loop_ref loop, std::unique_ptr<ProxyFactory> factory)
            : socket(loop, UnixSocket::type::stream)
            , factory(std::move(factory)) {}
    };

    ev::loop_ref &loop;

    std::list<Board> boards;

    std::list<Session> sessions;    // all sessions
    std::list<Session*> wait_queue; // sessions waiting for board

    ev::sig sigint_watcher { loop };
    ev::sig sigterm_watcher { loop };
    void on_signal(ev::sig &w, int revents);

    UnixSocket client_listener { loop, UnixSocket::type::seqpacket, true };
    std::list<std::unique_ptr<ProxyListener>> proxy_listeners;

    void on_client_connecting(ev::io &w, int revents);
    void on_socket_connecting(ev::io &w, int revents);

    template<void (Daemon::*method)(ev::io &w, int)>
    void setup_listener(UnixSocket &sock, std::string sock_name);

    Session *find_session_by_ppid(pid_t ppid);

    Board *find_available_board();
};

#endif // DAEMON_H
