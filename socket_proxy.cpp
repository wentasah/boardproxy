#include "socket_proxy.hpp"
#include "session.hpp"

SocketProxy::SocketProxy(const std::string proxy_type, Session &session, std::unique_ptr<UnixSocket> client_sock)
    : proxy_type(proxy_type)
    , session(session)
    , logger(session.logger)
    , client(move(client_sock))
    , id(session.proxy_cnt++)
{
}

void SocketProxy::close()
{
    return session.proxies.remove_if([&](auto &proxy) { return proxy.get() == this; });
}
