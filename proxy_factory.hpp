#ifndef PROXY_FACTORY_HPP
#define PROXY_FACTORY_HPP

#include <string>
#include <memory>
#include "socket_proxy.hpp"

class ProxyFactory {
public:
    const std::string sock_name;
    UnixSocket listener;

    ProxyFactory(ev::loop_ref loop, std::string sock_name)
        : sock_name(sock_name),  listener(loop, UnixSocket::type::stream) {};
    virtual ~ProxyFactory() {}

    virtual std::unique_ptr<SocketProxy>
    create(Session &session, std::unique_ptr<UnixSocket> client_sock) = 0;
};

#endif // PROXY_FACTORY_HPP
