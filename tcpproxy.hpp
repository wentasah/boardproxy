#ifndef TCPPROXY_HPP
#define TCPPROXY_HPP

#include <ev++.h>
#include <memory>
#include <array>
#include <vector>
#include <spdlog/logger.h>
#include "unix_socket.hpp"
#include "util.hpp"
#include "socket_proxy.hpp"
#include "proxy_factory.hpp"

class TcpProxy : public SocketProxy
{
public:
    TcpProxy(Session &session,
             std::unique_ptr<UnixSocket> client_sock, uint16_t port);
    ~TcpProxy() override;

private:
    ev::io target;

    using buffer_t = std::vector<char, default_init_allocator<char>>;
    buffer_t buf_c2t;
    buffer_t buf_t2c;

    void on_target_connected(ev::io &w, int revents);

    void on_client_data(ev::io &w, int revents) { on_data(client->watcher, target, buf_c2t, revents, "client", "target"); }
    void on_target_data(ev::io &w, int revents) { on_data(target, client->watcher, buf_t2c, revents, "target", "client"); }

    void on_data(ev::io &from, ev::io &to, buffer_t &buf, int revents,
                 const std::string_view from_name, const std::string_view to_name);
};

class TcpProxyFactory : public ProxyFactory {
public:
    TcpProxyFactory(ev::loop_ref loop, std::string sock_name, uint16_t port)
        : ProxyFactory(loop, sock_name), port(port) {}

    std::unique_ptr<SocketProxy>
    create(Session &session, std::unique_ptr<UnixSocket> client_sock) override {
        return std::make_unique<TcpProxy>(session, std::move(client_sock), port);
    }
private:
    uint16_t port;
};


#endif // TCPPROXY_HPP
