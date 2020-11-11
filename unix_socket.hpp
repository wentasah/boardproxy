#ifndef UNIX_SOCK_HPP
#define UNIX_SOCK_HPP

#include <string>
#include <ev++.h>
#include <memory>

class UnixSocket
{
public:
    enum class type { seqpacket, stream, datagram };

    UnixSocket(ev::loop_ref loop, type t, bool allow_socket_activation = false);
    ~UnixSocket();

    void connect(std::string path);
    void bind(std::string path);
    void listen();

    std::unique_ptr<UnixSocket> accept();

    void make_nonblocking();
    void close();

    struct ucred peer_cred();

    ev::io watcher;
    const bool is_from_systemd;
private:
    UnixSocket(ev::loop_ref loop, int fd);
};

#endif // UNIX_SOCK_HPP
