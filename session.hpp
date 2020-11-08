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
class WrProxy;

class Session {
public:
    Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket);
    ~Session();

    pid_t get_ppid() const { return ppid; }

    void assign_board(Board *brd);

    void new_wrproxy_connection(std::unique_ptr<UnixSocket> s);

    std::string get_status_line() const;
private:
    friend WrProxy;

    static uint64_t counter;
    const uint64_t id = { counter++ };

    std::shared_ptr<spdlog::logger> logger;

    Daemon &daemon;

    ev::loop_ref loop;

    std::unique_ptr<UnixSocket> client;
    Board *board = nullptr;

    // Data from the client
    int fd_in, fd_out, fd_err; // file descriptors
    pid_t ppid;
    std::string username_cred, username;

    std::array<char, 65536> buffer;
    void on_data_from_client(ev::io &w, int revents);
    void on_setup_msg(struct msghdr msg);

    ev::child child_watcher { loop };
    void start_process();
    void on_process_exit(ev::child &w, int revents);

    std::unique_ptr<WrProxy> wrproxy;

    void close_session();

    std::string get_username_cred();
};

#endif // CLIENT_H
