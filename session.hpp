#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <array>
#include <ev++.h>
#include <spdlog/logger.h>
#include "protocol.hpp"
#include "unix_socket.hpp"

class Daemon;

class Session {
public:
    Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket);
    ~Session();
private:
    std::shared_ptr<spdlog::logger> logger;

    Daemon &daemon;

    ev::loop_ref loop;

    std::unique_ptr<UnixSocket> client;

    // Data from the client
    int fd_in, fd_out, fd_err; // file descriptors
    pid_t ppid;
    std::string username;

    void on_data_from_client(ev::io &w, int revents);
    std::array<char, 65536> buffer;

    ev::child child_watcher { loop };
    void start_process();
    void on_process_exit(ev::child &w, int revents);

    void close_session();
};

#endif // CLIENT_H
