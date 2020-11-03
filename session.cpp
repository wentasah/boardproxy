#include <unistd.h>
#include <functional>
#include "session.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

Session::Session(ev::loop_ref loop, Daemon &daemon, std::unique_ptr<UnixSocket> socket)
    : daemon(daemon)
    , loop(loop)
    , client(std::move(socket))
{
    logger->info("New session {}", (void*)this);

    client->watcher.set<Session, &Session::on_data_from_client>(this);
    client->watcher.start();

    start_process();
}

Session::~Session()
{
    logger->info("Closing session {}", (void*)this);
}

void Session::on_data_from_client(ev::io &w, int revents)
{
    ssize_t ret = ::read(w.fd, &buffer, sizeof(buffer));
    if (ret == 0) {
        logger->info("Client closed connection");
        close_session();
    } else {
//        switch (header.id) {
//        case proto::msg_type::setup:
//            logger->info("Setup");
//            break;
//        }
//        start_reading_from_client();
    }
}

void Session::start_process()
{
    logger->info("Parent pid {}", getpid());

    pid_t pid = fork();
    if (pid == -1)
        throw std::system_error(errno, std::generic_category(), "fork");
    if (pid == 0) { // child
        execlp("sleep", "sleep", "10", NULL);
        throw std::system_error(errno, std::generic_category(), "exec");
    }
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
        ::kill(child_watcher.pid, SIGTERM);
    }
    daemon.close_session(this);
}
