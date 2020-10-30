#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include <boost/asio/local/stream_protocol.hpp>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using boost::asio::local::stream_protocol;

Daemon::Daemon(boost::asio::io_context &io, std::string sock_dir)
    : io(io)
    , acceptor(io)
{
    signals.async_wait(
        [&](std::error_code ec, int signo)
            {
                if (!ec) {
                    logger->info("Received {} signal", strsignal(signo));
                    io.stop();
                } else {
                    logger->error("Signal handling: {}", ec.message());
                }
            });

    mkdir(sock_dir.c_str(), S_IRWXU | S_IRWXG); // ignore erros
    auto path = sock_dir + "/boardproxy";
    unlink(path.c_str()); // ingore errors
    acceptor.open();
    acceptor.bind(stream_protocol::endpoint(path));
    acceptor.listen();

    accept_clients();
}

Daemon::~Daemon()
{
    logger->info("Closing daemon");
}

void Daemon::accept_clients()
{
    acceptor.async_accept(
        [this](std::error_code ec, stream_protocol::socket socket)
        {
          if (!ec) {
              sessions.emplace_back(io, *this, std::move(socket));
          } else {
              logger->error("Client accept error: {}", ec.message());
          }

          accept_clients();
        });
}


void Daemon::close_session(Session *session)
{
    sessions.remove_if([&](auto &s) { return &s == session; });
}
