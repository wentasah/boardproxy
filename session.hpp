#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <array>
#include <ev++.h>
#include <spdlog/logger.h>
#include "protocol.hpp"
#include "unix_socket.hpp"
#include "board.hpp"

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
    Board *board = nullptr;

    // Data from the client
    int fd_in, fd_out, fd_err; // file descriptors
    pid_t ppid;
    std::string username;

    std::array<char, 65536> buffer;
    void on_data_from_client(ev::io &w, int revents);
    void on_setup_msg(struct msghdr msg);

    ev::child child_watcher { loop };
    void start_process();
    void on_process_exit(ev::child &w, int revents);

    void close_session();

    Board *find_available_board();
};

#endif // CLIENT_H
