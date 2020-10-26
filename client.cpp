#include "client.hpp"
#include <asio.hpp>
#include "log.hpp"

using asio::local::stream_protocol;

Client::Client(asio::io_context &io, std::string sock_dir)
    : sock(io)
{
    sock.connect(stream_protocol::endpoint(sock_dir + "/boardproxy"));
    asio::async_read(sock, asio::buffer(buf), asio::transfer_all(),
                     [this](const asio::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            logger->error("Client read error: {}", ec.message());
        }
    });
}
