#include "client.hpp"
#include <boost/asio.hpp>
#include "log.hpp"

using boost::asio::local::stream_protocol;

Client::Client(boost::asio::io_context &io, std::string sock_dir)
    : sock(io)
{
    sock.connect(stream_protocol::endpoint(sock_dir + "/boardproxy"));
    boost::asio::async_read(sock, boost::asio::buffer(buf), boost::asio::transfer_all(),
                     [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            logger->error("Client read error: {}", ec.message());
        }
    });
}
