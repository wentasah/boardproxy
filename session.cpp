#include <unistd.h>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/program_options/parsers.hpp>
#include <cstdio>
#include "session.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"
#include "boards.hpp"
#include "wrproxy.hpp"

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
    if (board)
        board->release();
    logger->info("Closing session {}", (void*)this);
}

void Session::new_wrproxy_connection(std::unique_ptr<UnixSocket> s)
{
    wrproxy.reset(); // Deallocate old proxy (if any)
    wrproxy = make_unique<WrProxy>(*this, logger, move(s), board->ip_address);
}

void Session::on_data_from_client(ev::io &w, int revents)
{
    struct msghdr msg = { 0 };
    struct iovec io = {
        .iov_base = &buffer,
        .iov_len = sizeof(buffer)
    };
    union {         /* Ancillary data buffer, wrapped in a union
                       in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(sizeof(int[3]))];
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
        on_setup_msg(msg);
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

void Session::on_setup_msg(struct msghdr msg)
{
    int myfds[3]; // sent stdin/out/err from client
    bool fds_set = false;

    struct cmsghdr *cmsg = NULL;

    using namespace proto;

    auto s = reinterpret_cast<setup*>(&buffer);

    if (msg.msg_controllen < sizeof(myfds)) {
        logger->error("control len {} < {}", msg.msg_controllen, sizeof(myfds));
        close_session();
    }
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            memcpy(&myfds, CMSG_DATA(cmsg), sizeof(myfds));
            fds_set = true;
            break;
        }
    }
    if (cmsg == NULL || !fds_set) {
        logger->error("SCM_RIGHTS not received");
        close_session();
        return;
    }

    fd_in  = myfds[0];
    fd_out = myfds[1];
    fd_err = myfds[2];

    ppid = s->ppid;

    logger->info("ppid {}", s->ppid);

    board = find_available_board();

    if (!board) {
        dprintf(fd_err, "No board currently available. Please, try it later.\n");
        close_session();
        return;
    }

    board->acquire(this);
    start_process();
}

void Session::start_process()
{
    using namespace std;

    logger->info("Starting: {}", board->command);

    // Prepare command line arguments for exec()
    auto args = boost::program_options::split_unix(board->command);
    vector<const char*> cargs;
    transform(begin(args), end(args), back_inserter(cargs), [](auto &a) { return a.c_str(); });
    cargs.push_back(nullptr);

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

        execvp(cargs[0], const_cast<char**>(cargs.data()));
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
    logger->info("process exits with status {}", WEXITSTATUS(w.rstatus));
    close_session();
}

void Session::close_session()
{
    if (child_watcher.is_active()) {
        ::kill(child_watcher.pid, SIGKILL);
    }
    daemon.close_session(this);
}

Board *Session::find_available_board()
{
    for (auto &board : boards) {
        if (board.is_available())
            return &board;
    }
    return nullptr;
}
