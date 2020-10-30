#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>

// Client-side process (intended to be executed via SSH's ForceCommand)

class Client
{
public:
    Client(boost::asio::io_context &io, std::string sock_dir);
private:
    std::array<char, 128> buf;
    boost::asio::local::stream_protocol::socket sock;
};

#endif // CLIENT_HPP
