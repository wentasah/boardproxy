#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <system_error>
#include <algorithm>
#include "session.hpp"

#include "wrproxy.hpp"

using namespace  std;

WrProxy::WrProxy(Session &session, std::shared_ptr<spdlog::logger> logger, std::unique_ptr<UnixSocket> client_sock, std::string forced_ip)
    : session(session)
    , logger(logger)
    , client(move(client_sock))
    , target(client->watcher.loop)
{
    buf_c2t.reserve(65536);

    client->watcher.set<WrProxy, &WrProxy::on_client_data>(this);
    client->watcher.start();

    int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1)
        throw std::system_error(errno, std::generic_category(), "socket(SOCK_DGRAM)");
    target.set(sock, ev::READ);
    target.set<WrProxy, &WrProxy::on_target_data>(this);
    target.start();

    logger->info("wrproxy: New connection");
}

WrProxy::~WrProxy()
{
    logger->info("wrproxy: End of connection");
}

void WrProxy::on_client_data(ev::io &w, int revents)
{
    vector<char> &buf = buf_c2t;

    int ret = ::read(w.fd, buf.data() + buf.size(), buf.capacity() - buf.size());
    if (ret == -1) {
        logger->error("wrproxy: read error {}", strerror(errno));
        //TODO
        return;
    } else if (ret == 0) {
        logger->error("wrproxy: client closed the connection", strerror(errno));
        session.close_wrproxy();
        return;
    }
    buf.resize(buf.size() + ret);

    switch (mode) {
    case COMMAND:
        parse_command();
        break;
    case DATA:
        if (buf.size() > sizeof(uint32_t)) {
            uint32_t len = be32toh(*(uint32_t*)buf.data());
            if (buf.size() > sizeof(uint32_t) + len) {
                ret = ::write(target.fd, buf.data() + sizeof(uint32_t), len);
                if (ret == -1) {
                    logger->error("wrproxy: Target sock write error: {}", strerror(errno));
                    session.close_wrproxy();
                    return;
                }
            }
        }
        break;
    }

}

void WrProxy::on_target_data(ev::io &w, int revents)
{

}

void WrProxy::parse_command()
{
    string buf(begin(buf_c2t), end(buf_c2t));
    auto last = buf.find_first_of("\0\n\r");

    logger->info("wrproxy: XXX Received command: {}", buf);

    if (last == string::npos)
        return;

    // The following command is sent by WindRiver Workbench:
    // connect type=udpsock;tgt=10.35.95.50;port=17185;mode=packet;mode=packet

    logger->info("wrproxy: Received command: {}", buf);

    // TODO
}
