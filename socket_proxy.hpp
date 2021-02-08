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
    SocketProxy(const std::string proxy_type,
                Session &session,
                std::unique_ptr<UnixSocket> client_sock);
    virtual ~SocketProxy() {}
protected:
    const std::string proxy_type;
    Session &session;
    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<UnixSocket> client;
    const uint64_t id;

    template<typename FormatString, typename... Args>
    void info(const FormatString &fmt, const Args &... args) {
        logger->info("{} {}: {}", proxy_type, id, fmt::format(fmt, args...));
    }

    template<typename FormatString, typename... Args>
    void fail(const FormatString &fmt, const Args &... args) {
        logger->error("{} {}: {}", proxy_type, id, fmt::format(fmt, args...));
        return close();
    }

    void close();
};

#endif // SOCKET_HPP
