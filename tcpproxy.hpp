#ifndef TCPPROXY_HPP
#define TCPPROXY_HPP

#include <ev++.h>
#include <memory>
#include <array>
#include <vector>
#include <spdlog/logger.h>
#include "unix_socket.hpp"
#include "util.hpp"

class Session;

class TcpProxy
{
public:
    TcpProxy(Session &session, std::shared_ptr<spdlog::logger> logger,
             std::unique_ptr<UnixSocket> client_sock, uint16_t port);
    ~TcpProxy();

private:
    Session &session;
    const uint64_t id;
    std::shared_ptr<spdlog::logger> logger;

    std::unique_ptr<UnixSocket> client;
    ev::io target;

    using buffer_t = std::vector<char, default_init_allocator<char>>;
    buffer_t buf_c2t;
    buffer_t buf_t2c;

    void on_target_connected(ev::io &w, int revents);

    void on_client_data(ev::io &w, int revents) { on_data(client->watcher, target, buf_c2t, revents, "client", "target"); }
    void on_target_data(ev::io &w, int revents) { on_data(target, client->watcher, buf_t2c, revents, "target", "client"); }

    void on_data(ev::io &from, ev::io &to, buffer_t &buf, int revents,
                 const std::string_view from_name, const std::string_view to_name);

    template<typename FormatString, typename... Args>
    void info(const FormatString &fmt, const Args &... args);

    template<typename FormatString, typename... Args>
    void fail(const FormatString &fmt, const Args &... args);

    void close();
};

#endif // TCPPROXY_HPP
