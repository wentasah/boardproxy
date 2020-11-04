#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ev++.h>
#include <array>
#include "unix_socket.hpp"

// Client-side process (intended to be executed via SSH's ForceCommand)

class Client
{
public:
    Client(ev::loop_ref loop, std::string sock_dir);
    ~Client();

private:
    std::array<char, 65536> buffer;
    UnixSocket socket;

    void on_data_from_daemon(ev::io &w, int revents);

    void send_setup();
};

#endif // CLIENT_HPP
