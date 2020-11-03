#ifndef CLIENT_H
#define CLIENT_H

// Client representation inside the daemon

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/buffered_read_stream.hpp>
#include <vector>
#include <boost/process.hpp>
#include "protocol.hpp"

class Daemon;

class Session {
public:
    Session(boost::asio::io_context &io, Daemon &daemon,
           boost::asio::local::stream_protocol::socket sock);
    ~Session();
private:
    Daemon &daemon;

    boost::asio::io_context &io;
    boost::asio::local::stream_protocol::socket sock;

    void start_reading_from_client();
    proto::msg_header header;
    std::vector<char> data;

    boost::process::child child;
    void start_process();
    void on_process_exit(int exit, const std::error_code& ec_in);

    void close_session();
};

#endif // CLIENT_H
