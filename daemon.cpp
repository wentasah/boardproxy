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
    setup_listener<&Daemon::on_vxdbg_connecting> (vxdbg_listener,  sock_dir + "/vxdbg");

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

void Daemon::on_vxdbg_connecting(ev::io &w, int revents)
{
    auto socket = vxdbg_listener.accept();
    struct ucred cred;
    socklen_t len = sizeof(cred);
    int ret = getsockopt(socket->watcher.fd, SOL_SOCKET, SO_PEERCRED, &cred, &len);
    if (ret == -1) {
        logger->error("getsockopt(SO_PEERCRED): {}", strerror(errno));
        return;
    }

    logger->info("vxdbg connecting pid={}", cred.pid);
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
