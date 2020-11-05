#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using namespace std;

template<void (Daemon::*method)(ev::io &w, int)>
void Daemon::setup_listener(UnixSocket &sock, std::string sock_name)
{
    unlink(sock_name.c_str()); // ignore errors

    // Temporary hack until we make permissions configurable. It's
    // safe because on the server, we have tight directory permission.
    mode_t old_umask = umask(0);

    sock.bind(sock_name);
    umask(old_umask);
    sock.listen();

    sock.watcher.set<Daemon, method>(this);
    sock.watcher.start();
}

Daemon::Daemon(ev::loop_ref &io, std::string sock_dir)
    : loop(io)
{
    sigint_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigint_watcher.start(SIGINT);
    sigterm_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigterm_watcher.start(SIGTERM);

    mkdir(sock_dir.c_str(), S_IRWXU | S_IRWXG); // ignore erros

    setup_listener<&Daemon::on_client_connecting>(client_listener, sock_dir + "/boardproxy");
    setup_listener<&Daemon::on_wrproxy_connecting> (wrproxy_listener,  sock_dir + "/wrproxy");

    logger->info("Listening in {}", sock_dir);
}

Daemon::~Daemon()
{
    logger->info("Closing daemon");
}

void Daemon::on_client_connecting(ev::io &w, int revents)
{
    sessions.emplace_back(loop, *this, client_listener.accept());
}

void Daemon::on_wrproxy_connecting(ev::io &w, int revents)
{
    auto socket = wrproxy_listener.accept();
    struct ucred cred = socket->peer_cred();
    Session *s = find_session_by_ppid(cred.pid);
    if (s) {
        s->new_wrproxy_connection(std::move(socket));
    } else {
        logger->error("Cannot find session for wrproxy connection from pid {}", cred.pid);
    }
}

Session *Daemon::find_session_by_ppid(pid_t ppid)
{
    for (auto &s : sessions)
        if (s.get_ppid() == ppid)
            return &s;
    return nullptr;
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
