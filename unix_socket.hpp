#ifndef UNIX_SOCK_HPP
#define UNIX_SOCK_HPP

#include <string>
#include <ev++.h>
#include <memory>

class UnixSocket
{
public:
    UnixSocket(ev::loop_ref loop);
    ~UnixSocket();

    void connect(std::string path);
    void bind(std::string path);
    void listen();

    std::unique_ptr<UnixSocket> accept();

    void make_nonblocking();
    void close();

    ev::io watcher;
private:
    UnixSocket(ev::loop_ref loop, int fd);
};

#endif // UNIX_SOCK_HPP
