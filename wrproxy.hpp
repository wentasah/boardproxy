#ifndef WRPROXY_HPP
#define WRPROXY_HPP

#include <ev++.h>
#include <memory>
#include <array>
#include <vector>
#include <spdlog/logger.h>
#include "unix_socket.hpp"
#include "util.hpp"

class Session;

// Implementatin of (a subset of) WindRiver proxy protocol for
// connecting WindRiver Workbench IDE to VxWorks targets.

class WrProxy
{
public:
    WrProxy(Session &session, std::shared_ptr<spdlog::logger> logger,
            std::unique_ptr<UnixSocket> client_sock, std::string forced_ip);
    ~WrProxy();

private:
    Session &session;
    std::shared_ptr<spdlog::logger> logger;

    enum { COMMAND, DATA } state = COMMAND;

    std::vector<char, default_init_allocator<char>> buf_c2t;
    std::array<char, 65535> buf_t2c;
    std::unique_ptr<UnixSocket> client;
    ev::io target;

    void on_client_data(ev::io &w, int revents);
    void on_target_data(ev::io &w, int revents);

    void parse_command();
    void handle_connect(std::istringstream &cmd);

    template<typename FormatString, typename... Args>
    void fail(const FormatString &fmt, const Args &... args);

    void close();
};

#endif // WRPROXY_HPP
