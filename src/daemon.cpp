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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include "version.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using namespace std;

template<void (Daemon::*method)(ev::io &w, int)>
void Daemon::setup_listener(UnixSocket &sock, std::string sock_name)
{
    if (!sock.is_from_systemd) {
        unlink(sock_name.c_str()); // ignore errors
        sock.bind(sock_name);
        sock.listen();
    }

    sock.watcher.set<Daemon, method>(this);
    sock.watcher.start();
}

Daemon::Daemon(ev::loop_ref &io, std::string sock_dir, std::list<Board> boards, std::list<std::unique_ptr<ProxyFactory> > proxy_factories)
    : loop(io)
    , boards(move(boards))
{
    logger->info("boardproxy version {}", boardproxy_version);

    sigint_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigint_watcher.start(SIGINT);
    sigterm_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigterm_watcher.start(SIGTERM);

    int ret = mkdir(sock_dir.c_str(), S_IRWXU | S_IRWXG);
    if (ret == -1 && errno == EEXIST) {
        // Directory already exists - ignore it
    } else if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "sock_dir " + sock_dir);
    }

    setup_listener<&Daemon::on_client_connecting>(client_listener, sock_dir + "/boardproxy");

    for (auto &factory : proxy_factories) {
        proxy_listeners.push_back(make_unique<ProxyListener>(loop, move(factory)));
        setup_listener<&Daemon::on_socket_connecting>(proxy_listeners.back()->socket,
                                                      sock_dir + "/" + proxy_listeners.back()->factory->sock_name);
    }
    if (client_listener.is_from_systemd)
        logger->info("Activated by systemd, listening in {}", sock_dir);
    else
        logger->info("Listening in {}", sock_dir);
}

Daemon::~Daemon()
{
    logger->info("Closing daemon");
}

void Daemon::assign_board(Session *session)
{
    Board *brd = find_available_board(session->get_username());

    if (brd) {
        // Notify the session about board availability
        session->assign_board(*brd);
    } else {
        wait_queue.push_back(session);
    }
}

void Daemon::on_client_connecting(ev::io &w, int revents)
{
    sessions.emplace_back(loop, *this, client_listener.accept());
}

void Daemon::on_socket_connecting(ev::io &w, int revents)
{
    ProxyListener *listener = nullptr;
    for (auto &l : proxy_listeners) {
        // TODO: get rid of O(n) complexity
        if (&(l->socket.watcher) == &w) {
            listener = l.get();
            break;
        }
    }
    if (!listener)
        throw std::runtime_error(string("Proxy factory not found in ") + string(__PRETTY_FUNCTION__));

    auto socket = listener->socket.accept();
    struct ucred cred = socket->peer_cred();
    Session *s = find_session_by_ppid(cred.pid);
    if (s) {
        s->new_socket_connection(move(socket), *listener->factory.get());
    } else {
        logger->error("Cannot find session for {} connection from pid {}",
                      listener->factory->sock_name, cred.pid);
    }
}

Session *Daemon::find_session_by_ppid(pid_t ppid)
{
    for (auto &s : sessions)
        if (s.get_ppid() == ppid)
            return &s;
    return nullptr;
}

Board *Daemon::find_available_board(const string& username)
{
    for (auto &board : boards) {
        if (board.is_available(username))
            return &board;
    }
    return nullptr;
}


void Daemon::close_session(Session *session)
{
    wait_queue.remove(session);
    sessions.remove_if([&](auto &s) { return &s == session; });

    // Go through waiting users if some of them can get the freed board.
    for (Session *waiting : wait_queue) {
        Board *board = find_available_board(waiting->get_username());
        if (board) {
            wait_queue.remove(waiting);
            waiting->assign_board(*board);
            break;
        }
    }

    if (client_listener.is_from_systemd && sessions.empty()) {
        logger->info("No session active");
        loop.break_loop(); // exit
    }
}

void Daemon::print_status(int fd)
{
    for (const Session &sess: sessions) {
        if (sess.get_status() != Session::status::created) {
            string str = sess.get_status_line();
            dprintf(fd, "%s\n", str.c_str());
        }
    }
}

void Daemon::on_signal(ev::sig &w, int revents)
{
    logger->info("Received {} signal", strsignal(w.signum));
    loop.break_loop(ev::ALL);
}
