#include "client.hpp"
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <functional>
#include "log.hpp"

using boost::asio::local::stream_protocol;

Client::Client(boost::asio::io_context &io, std::string sock_dir)
    : sock(io)
{
    sock.connect(stream_protocol::endpoint(sock_dir + "/boardproxy"));
    start_sock_read();
}

void Client::start_sock_read()
{
    using namespace std::placeholders;  // for _1, _2, _3...
    boost::asio::async_read(sock, boost::asio::buffer(buf), boost::asio::transfer_all(),
                            std::bind(&Client::on_daemon_read, this, _1, _2));
}

void Client::on_daemon_read(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    if (ec == boost::asio::error::eof) {
        logger->info("Server connection closed");
        return;
    }
    if (ec) {
        logger->error("Client read error: {} {} {}", ec.category().name(), ec.value(), ec.message());
        return;
    }
    start_sock_read();
}
