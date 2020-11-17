#ifndef DAEMON_H
#define DAEMON_H

#include <list>
#include <string>
#include <ev++.h>
#include "session.hpp"
#include "unix_socket.hpp"
#include "board.hpp"

class Daemon {
public:
    Daemon(ev::loop_ref &loop, std::string sock_dir, std::list<Board> boards);
    ~Daemon();
    void run();
    void assign_board(Session *session);
    void close_session(Session *session);

    void print_status(int fd);
private:
    ev::loop_ref &loop;

    std::list<Board> boards;

    std::list<Session> sessions;    // all sessions
    std::list<Session*> wait_queue; // sessions waiting for board

    ev::sig sigint_watcher { loop };
    ev::sig sigterm_watcher { loop };
    void on_signal(ev::sig &w, int revents);

    UnixSocket client_listener { loop, UnixSocket::type::seqpacket, true };
    UnixSocket wrproxy_listener { loop, UnixSocket::type::stream };
    UnixSocket www_listener { loop, UnixSocket::type::stream };

    void on_client_connecting(ev::io &w, int revents);
    void on_wrproxy_connecting(ev::io &w, int revents);
    void on_www_connecting(ev::io &w, int revents);

    template<void (Daemon::*method)(ev::io &w, int)>
    void setup_listener(UnixSocket &sock, std::string sock_name);

    Session *find_session_by_ppid(pid_t ppid);

    Board *find_available_board();
};

#endif // DAEMON_H
