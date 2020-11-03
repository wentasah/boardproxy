#include <functional>
#include "client.hpp"
#include "log.hpp"
#include "unix_socket.hpp"

Client::Client(ev::loop_ref loop, std::string sock_dir)
    : socket(loop)
{
    socket.connect(sock_dir + "/boardproxy");
    socket.watcher.set<Client, &Client::on_data_from_daemon>(this);
    socket.watcher.start();
}

Client::~Client()
{
}

void Client::on_data_from_daemon(ev::io &w, int revents)
{
    ssize_t ret = ::read(w.fd, &buffer, sizeof(buffer));
    if (ret == 0) {
        logger->info("Server closed connection");
        w.stop();
    }
    logger->error("Client read error: {}", strerror(ENOSYS));
}
