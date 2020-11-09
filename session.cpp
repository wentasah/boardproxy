#include <unistd.h>
#include <functional>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/program_options/parsers.hpp>
#include <cstdio>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include "session.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"
#include "wrproxy.hpp"

using namespace std;

uint64_t Session::counter = 0;

Session::Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket)
    : logger(spdlog::stderr_color_st(fmt::format("session {}", id)))
    , daemon(daemon)
    , loop(loop)
    , client(std::move(socket))
    , username_cred(get_username_cred())
    , session_since(chrono::system_clock::to_time_t(chrono::system_clock::now()))
{
    logger->info("New session ({})", username_cred);

    client->watcher.set<Session, &Session::on_data_from_client>(this);
    client->watcher.start();
}

Session::~Session()
{
    if (board)
        board->release();

    ::close(fd_in);
    ::close(fd_out);
    ::close(fd_err);

    logger->info("Closing session ({})", username_cred);
}

void Session::new_wrproxy_connection(std::unique_ptr<UnixSocket> s)
{
    wrproxy.reset(); // Deallocate old proxy (if any)
    wrproxy = make_unique<WrProxy>(*this, logger, move(s), board->ip_address);
}

string Session::get_status_line() const
{
    return fmt::format(("{:10s} {:15s} {:%c}"),
                       username.empty() ? username_cred : username,
                       board ? board->ip_address : "waiting",
                       fmt::localtime(board ? board_since : session_since));
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
    username = s->username;

    logger->debug("Setting up: ppid={}, username={}",
                  ppid, username);

    switch (s->cmd) {
    case setup::command::connect:
        status = status::awaiting_board;
        // Call this->assign_board with either a board or nullptr
        daemon.assign_board(this);
        break;
    case setup::command::list_sessions:
        daemon.print_status(fd_out);
        return close_session();
    };
}

void Session::assign_board(Board *brd)
{
    if (brd) {
        status = status::has_board;
        board = brd;
        board_since = chrono::system_clock::to_time_t(chrono::system_clock::now());
        board->acquire(this);
        logger->info("Associated with board {}", board->ip_address);
        dprintf(fd_err, "Connecting to board %s\n", board->ip_address.c_str());

        start_process();
    } else {
        logger->warn("No board currently available");
        daemon.print_status(fd_err);
        dprintf(fd_err, "No board currently available. Waiting...\n");
    }
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

string Session::get_username_cred()
{
    auto cred = client->peer_cred();

    long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1)          /* Value was indeterminate */
        bufsize = 16384;        /* Should be more than enough */
    vector<char> buf(bufsize);
    struct passwd pwd, *result;
    int err = getpwuid_r(cred.uid, &pwd, buf.data(), buf.size(), &result);
    if (err != 0) {
        logger->error("getpwuid error: {}", strerror(err));
    } else if (result == NULL) {
        logger->error("No pwd entry for UID {}", cred.uid);
    } else {
        return result->pw_name;
    }
    return "";
}
