#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <asio/io_context.hpp>
#include <asio/local/stream_protocol.hpp>

// Client-side process (intended to be executed via SSH's ForceCommand)

class Client
{
public:
    Client(asio::io_context &io, std::string sock_dir);
private:
    std::array<char, 128> buf;
    asio::local::stream_protocol::socket sock;
};

#endif // CLIENT_HPP
