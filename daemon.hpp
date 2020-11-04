#ifndef DAEMON_H
#define DAEMON_H

#include <list>
#include <string>
#include <ev++.h>
#include "session.hpp"
#include "unix_socket.hpp"

class Daemon {
public:
    Daemon(ev::loop_ref &loop, std::string sock_dir);
    ~Daemon();
    void run();
    void close_session(Session *session);
private:
    ev::loop_ref &loop;
    std::list<Session> sessions;

    ev::sig sigint_watcher { loop };
    ev::sig sigterm_watcher { loop };
    void on_signal(ev::sig &w, int revents);

    UnixSocket client_listener { loop };
    UnixSocket vxdbg_listener { loop };

    void on_client_connecting(ev::io &w, int revents);
    void on_vxdbg_connecting(ev::io &w, int revents);

    template<void (Daemon::*method)(ev::io &w, int)>
    void setup_listener(UnixSocket &sock, std::string sock_name);
};

#endif // DAEMON_H
