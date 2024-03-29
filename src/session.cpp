// Copyright (C) 2021, 2022 Michal Sojka <michal.sojka@cvut.cz>
// 
// This file is part of boardproxy.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "tcpproxy.hpp"

using namespace std;

uint64_t Session::counter = 0;

Session::Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket)
    : client(std::move(socket))
    , username_cred(get_username_cred(*client))
    , logger(spdlog::stderr_color_st(fmt::format("session {} {}", id, username_cred)))
    , daemon(daemon)
    , loop(loop)
    , session_since(chrono::system_clock::to_time_t(chrono::system_clock::now()))
{
    logger->debug("Creating session");

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

    if (status != status::created)
        logger->info("Closing session");
    else
        logger->debug("Destroying session");
}

void Session::new_socket_connection(std::unique_ptr<UnixSocket> s, ProxyFactory &proxy_factory)
{
    if (this->status != status::has_board) {
        logger->warn("{} connection without a board", proxy_factory.sock_name);
        return;
    }
    proxies.push_back(proxy_factory.create(*this, move(s)));
}

string Session::get_username() const
{
    // TODO: Add trust_username option to the config file and user
    // this->username only if trusted.
    //
    // We cannot always rely on username_cred, which cannot be faked,
    // because in some setups, a single UNIX account is used by
    // multiple users. In this case, the --name option (stored in
    // this->username) can be used to distinguish between those users.
    // If the arguments of the --name are specified by the system
    // administrator (e.g. in ~/.ssh/authorized_keys), we can trust
    // them.
    //
    // In other setups, when users can specify the --name arbitrarily,
    // we should not use that value.
    return username.empty() ? username_cred : username;
}

string Session::get_status_line() const
{
    return fmt::format(("{:10s} {:10s} {:15s} {:%c}"),
                       get_username(),
                       board ? board->id : "",
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
    if (ret == -1) {
        logger->error("Client recvmsg error: {}", strerror(errno));
        client->watcher.stop();
        return close_session();
    }
    if (ret == 0) {
        logger->info("Client closed connection");
        client->watcher.stop();
        return close_session();
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
        logger->error("Unknown message type {}", static_cast<int>(h.id));
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
        logger->info("Client connected");
        // Call this->assign_board with either a board or nullptr
        daemon.assign_board(this);
        if (status == status::awaiting_board) {
            logger->warn("No board currently available");
            daemon.print_status(fd_err);
            dprintf(fd_err, "No board currently available.%s\n",
                    s->no_wait ? "" : " Waiting...");
            if (s->no_wait)
                return close_session();
        }
        break;
    case setup::command::list_sessions:
        daemon.print_status(fd_out);
        return close_session();
    };
}

void Session::assign_board(Board &brd)
{
    status = status::has_board;
    board = &brd;
    board_since = chrono::system_clock::to_time_t(chrono::system_clock::now());
    board->acquire(this);
    logger->info("Associated with board {} ({})", board->id, board->ip_address);
    dprintf(fd_err, "Connecting to board %s\n", board->id.c_str());

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
    fd_in = fd_out = fd_err = -1;
    child_watcher.set<Session, &Session::on_process_exit>(this);
    child_watcher.start(pid);
}

void Session::on_process_exit(ev::child &w, int revents)
{
    logger->info("process exits with status {}", WEXITSTATUS(w.rstatus));
    w.stop();
    close_session();
}

void Session::start_close_command()
{
    using namespace std;

    logger->info("Starting close command: {}", board->close_command);

    // Prepare command line arguments for exec()
    auto args = boost::program_options::split_unix(board->close_command);
    vector<const char*> cargs;
    transform(begin(args), end(args), back_inserter(cargs), [](auto &a) { return a.c_str(); });
    cargs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1)
        throw std::system_error(errno, std::generic_category(), "fork");
    if (pid == 0) { // child
        execvp(cargs[0], const_cast<char**>(cargs.data()));
        throw std::system_error(errno, std::generic_category(), "exec");
    }
    child_watcher.set<Session, &Session::on_close_command_exit>(this);
    child_watcher.start(pid);
}

void Session::on_close_command_exit(ev::child &w, int revents)
{
    logger->info("close command exits with status {}", WEXITSTATUS(w.rstatus));
    w.stop();
    close_session();
}

void Session::close_session()
{
    // This implements simple "board closing" state machine
    if (status == status::has_board && child_watcher.is_active()) {
        ::kill(child_watcher.pid, SIGKILL);
        return;
    } else if (status == status::has_board && !board->close_command.empty()) {
        status = status::closing_board;
        start_close_command();
        return;
    }
    daemon.close_session(this);
}

string Session::get_username_cred(UnixSocket &client)
{
    auto cred = client.peer_cred();

    long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1)          /* Value was indeterminate */
        bufsize = 16384;        /* Should be more than enough */
    vector<char> buf(bufsize);
    struct passwd pwd, *result;
    int err = getpwuid_r(cred.uid, &pwd, buf.data(), buf.size(), &result);
    if (err != 0) {
        ::logger->error("getpwuid error: {}", strerror(err));
    } else if (result == NULL) {
        ::logger->error("No pwd entry for UID {}", cred.uid);
    } else {
        return result->pw_name;
    }
    return "";
}
