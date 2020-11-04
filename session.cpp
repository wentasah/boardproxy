#include <unistd.h>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include "session.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using namespace std;

Session::Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket)
    : logger(spdlog::stderr_color_st(fmt::format("session {}", (void*)this)))
    , daemon(daemon)
    , loop(loop)
    , client(std::move(socket))
{
    logger->info("New session", (void*)this);

    client->watcher.set<Session, &Session::on_data_from_client>(this);
    client->watcher.start();
}

Session::~Session()
{
    logger->info("Closing session {}", (void*)this);
}

void Session::on_data_from_client(ev::io &w, int revents)
{
    int myfds[3]; // sent stdin/out/err from client

    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg = NULL;
    struct iovec io = {
        .iov_base = &buffer,
        .iov_len = sizeof(buffer)
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

    ssize_t ret = ::recvmsg(w.fd, &msg, 0);
    if (ret == 0) {
        logger->info("Client closed connection");
        close_session();
        return;
    }
    proto::header h;
    if (ret < ssize_t(sizeof(h))) {
        logger->error("Short packet received: len={}", ret);
        return;
    }

    memcpy((void*)&h, &buffer, sizeof(h));
    using namespace proto;
    switch (h.id) {
    case msg_type::setup: {
        auto s = reinterpret_cast<setup*>(&buffer);

        if (msg.msg_controllen < sizeof(myfds)) {
            logger->error("control len {} < {}", msg.msg_controllen, sizeof(myfds));
            close_session();
        }
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
                memcpy(&myfds, CMSG_DATA(cmsg), sizeof(myfds));
                break;
            }
        }
        if (cmsg == NULL) {
            logger->error("SCM_RIGHTS not received");
            close_session();
        }

        fd_in  = myfds[0];
        fd_out = myfds[1];
        fd_err = myfds[2];

        ppid = s->ppid;

        logger->info("ppid {}", s->ppid);

        start_process();
        break;
    }
    default:
        logger->error("Unknown message type {}", h.id);
    }

//        switch (header.id) {
//        case proto::msg_type::setup:
//            logger->info("Setup");
//            break;
//        }
//        start_reading_from_client();
}

void Session::start_process()
{
    logger->info("Parent pid {}", getpid());

    pid_t pid = fork();
    if (pid == -1)
        throw std::system_error(errno, std::generic_category(), "fork");
    if (pid == 0) { // child
        dup2(fd_in, STDIN_FILENO);
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_in);
        close(fd_out);
        close(fd_err);

        execlp("bash", "bash", NULL);
        throw std::system_error(errno, std::generic_category(), "exec");
    }
    close(fd_in);
    close(fd_out);
    close(fd_err);
    child_watcher.set<Session, &Session::on_process_exit>(this);
    child_watcher.start(pid);
}

void Session::on_process_exit(ev::child &w, int revents)
{
    logger->info("process exits: {}", w.rstatus);
    close_session();
}

void Session::close_session()
{
    if (child_watcher.is_active()) {
        ::kill(child_watcher.pid, SIGKILL);
    }
    daemon.close_session(this);
}
