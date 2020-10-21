#ifndef DAEMON_H
#define DAEMON_H

#include <list>
#include <string>
#include <asio/io_context.hpp>
#include <asio/local/stream_protocol.hpp>
#include "client.hpp"

class Daemon {
public:
    Daemon(asio::io_context &io, std::string sock_dir);
    void run();
private:
    asio::io_context &io;
    std::list<Client> clients;

    asio::local::stream_protocol::acceptor acceptor;

    void accept_clients();
    //void on_client_connected (ev::io &w, int revents);
};

#endif // DAEMON_H
