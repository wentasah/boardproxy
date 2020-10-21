#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <asio/io_context.hpp>
#include <asio/local/stream_protocol.hpp>
#include <asio/buffered_read_stream.hpp>
#include <vector>
#include "protocol.hpp"

class Daemon;

class Client {
public:
    Client(asio::io_context &io, Daemon &daemon,
           asio::local::stream_protocol::socket sock);
private:
    Daemon &daemon;

    asio::io_context &io;
    asio::local::stream_protocol::socket sock;

    void start_reading_from_client();
    proto::msg_header header;
    std::vector<char> data;
};

#endif // CLIENT_H