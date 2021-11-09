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
#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <array>
#include <ev++.h>
#include <spdlog/logger.h>
#include <ctime>
#include <list>
#include "protocol.hpp"
#include "unix_socket.hpp"
#include "board.hpp"
#include "proxy_factory.hpp"
#include "socket_proxy.hpp"

class Daemon;

class Session {
public:
    enum class status { created, awaiting_board, has_board, closing_board };

    Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket);
    ~Session();

    pid_t get_ppid() const { return ppid; }
    enum status get_status() const { return status; }

    void assign_board(Board &brd);

    void new_socket_connection(std::unique_ptr<UnixSocket> s, ProxyFactory &proxy_factory);
    const Board* get_board() const { return board; }

    std::string get_username() const;
    std::string get_status_line() const;
private:
    friend SocketProxy;

    static uint64_t counter;
    const uint64_t id = { counter++ };

    std::unique_ptr<UnixSocket> client;
    std::string username_cred;

    enum status status = status::created;


    std::shared_ptr<spdlog::logger> logger;

    Daemon &daemon;

    ev::loop_ref loop;

    Board *board = nullptr;

    // Data from the client
    int fd_in = -1, fd_out = -1, fd_err = -1; // file descriptors
    pid_t ppid;
    std::string username;

    std::array<char, 65536> buffer;
    void on_data_from_client(ev::io &w, int revents);
    void on_setup_msg(struct msghdr msg);

    std::time_t session_since, board_since;

    ev::child child_watcher { loop };
    void start_process();
    void on_process_exit(ev::child &w, int revents);
    void start_close_command();
    void on_close_command_exit(ev::child &w, int revents);

    std::list<std::unique_ptr<SocketProxy>> proxies;
    uint64_t proxy_cnt = 0;

    void close_session();

    static std::string get_username_cred(UnixSocket &client);
};

#endif // CLIENT_H
