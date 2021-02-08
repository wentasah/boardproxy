#ifndef WRPROXY_HPP
#define WRPROXY_HPP

#include <ev++.h>
#include <memory>
#include <array>
#include <vector>
#include <spdlog/logger.h>
#include "unix_socket.hpp"
#include "util.hpp"
#include "socket_proxy.hpp"
#include "proxy_factory.hpp"

// Implementation of (a subset of) WindRiver proxy protocol for
// connecting WindRiver Workbench IDE to VxWorks targets.

class WrProxy : public SocketProxy
{
public:
    WrProxy(Session &session, std::unique_ptr<UnixSocket> client_sock);
    ~WrProxy() override;

private:
    enum { COMMAND, DATA } state = COMMAND;

    std::vector<char, default_init_allocator<char>> buf_c2t;
    std::array<char, 65535> buf_t2c;
    ev::io target {client->watcher.loop};

    void on_client_data(ev::io &w, int revents);
    void on_target_data(ev::io &w, int revents);

    void parse_command();
    void handle_connect(std::istringstream &cmd);

    template<typename FormatString, typename... Args>
    void fail(const FormatString &fmt, const Args &... args);
};

class WrProxyFactory : public ProxyFactory {
public:
    using ProxyFactory::ProxyFactory;
    std::unique_ptr<SocketProxy>
    create(Session &session, std::unique_ptr<UnixSocket> client_sock) override {
        return std::make_unique<WrProxy>(session, std::move(client_sock));
    }
};

#endif // WRPROXY_HPP
