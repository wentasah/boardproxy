#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include "debug.hpp"
#include "daemon.hpp"
#include <asio/signal_set.hpp>
#include <asio/local/stream_protocol.hpp>
#include "log.hpp"

using asio::local::stream_protocol;

Daemon::Daemon(asio::io_context &io, std::string sock_dir)
    : io(io)
    , acceptor(io, stream_protocol::endpoint(sock_dir + "/boardproxy"))
{
    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait(
        [&](std::error_code /*ec*/, int /*signo*/)
            {
                io.stop();
            });

    accept_clients();
}

void Daemon::accept_clients()
{
    acceptor.async_accept(
        [this](std::error_code ec, stream_protocol::socket socket)
        {
          if (!ec) {
              logger->info("Client accepted");
              clients.emplace_back(io, *this, std::move(socket));
          } else {
              logger->error("Client accept error {}", ec);
          }

          accept_clients();
        });
}
