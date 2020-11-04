#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

Daemon::Daemon(ev::loop_ref &io, std::string sock_dir)
    : loop(io)
{
    sigint_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigint_watcher.start(SIGINT);
    sigterm_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigterm_watcher.start(SIGTERM);

    mkdir(sock_dir.c_str(), S_IRWXU | S_IRWXG); // ignore erros
    auto path = sock_dir + "/boardproxy";
    unlink(path.c_str()); // ingore errors

    client_listener.bind(path);
    client_listener.listen();

    client_listener.watcher.set<Daemon, &Daemon::on_client_connecting>(this);
    client_listener.watcher.start();

    logger->info("Listening on {}", path);
}

Daemon::~Daemon()
{
    logger->info("Closing daemon");
}

void Daemon::on_client_connecting(ev::io &w, int revents)
{
    sessions.emplace_back(loop, *this, client_listener.accept());
}


void Daemon::close_session(Session *session)
{
    sessions.remove_if([&](auto &s) { return &s == session; });
}

void Daemon::on_signal(ev::sig &w, int revents)
{
    logger->info("Received {} signal", strsignal(w.signum));
    loop.break_loop(ev::ALL);
}
