#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <memory>
#include <spdlog/logger.h>
#include "unix_socket.hpp"

class Session;

/// Abstract base class for proxied sockets
class SocketProxy
{
public:
    //SocketProxy(Session &session, std::shared_ptr<spdlog::logger> logger, std::unique_ptr<UnixSocket> client_sock);
    virtual ~SocketProxy() {}

};

#endif // SOCKET_HPP
