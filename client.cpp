#include <functional>
#include <sys/socket.h>
#include "client.hpp"
#include "log.hpp"
#include "unix_socket.hpp"
#include "protocol.hpp"


Client::Client(ev::loop_ref loop, std::string sock_dir)
    : socket(loop)
{
    socket.connect(sock_dir + "/boardproxy");
    socket.watcher.set<Client, &Client::on_data_from_daemon>(this);
    socket.watcher.start();

    send_setup();
}

Client::~Client()
{
}

void Client::on_data_from_daemon(ev::io &w, int revents)
{
    ssize_t ret = ::read(w.fd, &buffer, sizeof(buffer));
    if (ret == 0) {
        logger->info("Server closed connection");
        w.stop();
    }
    logger->error("Client read error: {}", strerror(ENOSYS));
}

void Client::send_setup()
{
    // Send stdin/out/err to the daemon so that it can run the proxied
    // process on them
    int myfds[3] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
    // Also send addition data, needed by the daemon
    proto::setup data(getppid());

    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg;
    struct iovec io = {
        .iov_base = &data,
        .iov_len = sizeof(data)
    };
    union {         /* Ancillary data buffer, wrapped in a union
                       in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(sizeof(myfds))];
        struct cmsghdr align;
    } u;

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(myfds));
    memcpy(CMSG_DATA(cmsg), myfds, sizeof(myfds));

    ::sendmsg(socket.watcher.fd, &msg, 0);

    close(STDIN_FILENO);
}
