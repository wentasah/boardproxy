#ifndef DAEMON_H
#define DAEMON_H

#include <list>
#include <string>
#include <asio/io_context.hpp>
#include <asio/local/stream_protocol.hpp>
#include <asio/signal_set.hpp>
#include "session.hpp"

class Daemon {
public:
    Daemon(asio::io_context &io, std::string sock_dir);
    ~Daemon();
    void run();
    void close_session(Session *session);
private:
    asio::io_context &io;
    std::list<Session> sessions;

    asio::signal_set signals {io, SIGINT, SIGTERM};
    asio::local::stream_protocol::acceptor acceptor;

    void accept_clients();
    //void on_client_connected (ev::io &w, int revents);

};

#endif // DAEMON_H
