#ifndef PROXY_FACTORY_HPP
#define PROXY_FACTORY_HPP

#include <string>
#include <memory>
#include "socket_proxy.hpp"

class ProxyFactory {
public:
    const std::string sock_name;

    ProxyFactory(std::string sock_name) : sock_name(sock_name) {}
    virtual ~ProxyFactory() {}

    virtual std::unique_ptr<SocketProxy>
    create(Session &session, std::unique_ptr<UnixSocket> client_sock) = 0;
};

#endif // PROXY_FACTORY_HPP
