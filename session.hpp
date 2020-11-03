#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <array>
#include <ev++.h>
#include "protocol.hpp"
#include "unix_socket.hpp"

class Daemon;

class Session {
public:
    Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket);
    ~Session();
private:
    Daemon &daemon;

    ev::loop_ref loop;

    std::unique_ptr<UnixSocket> client;

    void on_data_from_client(ev::io &w, int revents);
    proto::msg_header header;
    std::array<char, 65536> buffer;

    ev::child child_watcher { loop };
    void start_process();
    void on_process_exit(ev::child &w, int revents);

    void close_session();
};

#endif // CLIENT_H
