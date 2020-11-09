#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ev++.h>
#include <array>
#include "unix_socket.hpp"
#include "protocol.hpp"

// Client-side process (intended to be executed via SSH's ForceCommand)

class Client
{
public:
    Client(ev::loop_ref loop, std::string sock_dir,
           proto::setup::command_t command,
           std::string username);
    ~Client();

private:
    std::array<char, 65536> buffer;
    UnixSocket socket;

    const bool is_connect;

    void on_data_from_daemon(ev::io &w, int revents);

    void send_setup(proto::setup::command_t command,
                    const std::string &username);
};

#endif // CLIENT_HPP
