#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <err.h>
#include <unistd.h>
#include "version.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using namespace std;

template<void (Daemon::*method)(ev::io &w, int)>
void Daemon::setup_listener(UnixSocket &sock, std::string sock_name)
{
    if (!sock.is_from_systemd) {
        unlink(sock_name.c_str()); // ignore errors
        sock.bind(sock_name);
        sock.listen();
    }

    sock.watcher.set<Daemon, method>(this);
    sock.watcher.start();
}

Daemon::Daemon(ev::loop_ref &io, std::string sock_dir, std::list<Board> boards, std::list<std::unique_ptr<ProxyFactory> > proxy_factories)
    : loop(io)
    , boards(move(boards))
{
    logger->info("boardproxy version {}", boardproxy_version);

    sigint_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigint_watcher.start(SIGINT);
    sigterm_watcher.set<Daemon, &Daemon::on_signal>(this);
    sigterm_watcher.start(SIGTERM);

    int ret = mkdir(sock_dir.c_str(), S_IRWXU | S_IRWXG);
    if (ret == -1 && errno == EEXIST) {
        // Directory already exists - ignore it
    } else if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "sock_dir " + sock_dir);
    }

    setup_listener<&Daemon::on_client_connecting>(client_listener, sock_dir + "/boardproxy");

    for (auto &factory : proxy_factories) {
        proxy_listeners.push_back(make_unique<ProxyListener>(loop, move(factory)));
        setup_listener<&Daemon::on_socket_connecting>(proxy_listeners.back()->socket,
                                                      sock_dir + "/" + proxy_listeners.back()->factory->sock_name);
    }
    if (client_listener.is_from_systemd)
        logger->info("Activated by systemd, listening in {}", sock_dir);
    else
        logger->info("Listening in {}", sock_dir);
}

Daemon::~Daemon()
{
    logger->info("Closing daemon");
}

void Daemon::assign_board(Session *session)
{
    Board *brd = find_available_board();

    // Notify the session about board availability
    session->assign_board(brd);

    if (!brd)
        wait_queue.push_back(session);
}

void Daemon::on_client_connecting(ev::io &w, int revents)
{
    sessions.emplace_back(loop, *this, client_listener.accept());
}

void Daemon::on_socket_connecting(ev::io &w, int revents)
{
    ProxyListener *listener = nullptr;
    for (auto &l : proxy_listeners) {
        // TODO: get rid of O(n) complexity
        if (&(l->socket.watcher) == &w) {
            listener = l.get();
            break;
        }
    }
    if (!listener)
        throw std::runtime_error(string("Proxy factory not found in ") + string(__PRETTY_FUNCTION__));

    auto socket = listener->socket.accept();
    struct ucred cred = socket->peer_cred();
    Session *s = find_session_by_ppid(cred.pid);
    if (s) {
        s->new_socket_connection(move(socket), *listener->factory.get());
    } else {
        logger->error("Cannot find session for {} connection from pid {}",
                      listener->factory->sock_name, cred.pid);
    }
}

Session *Daemon::find_session_by_ppid(pid_t ppid)
{
    for (auto &s : sessions)
        if (s.get_ppid() == ppid)
            return &s;
    return nullptr;
}

Board *Daemon::find_available_board()
{
    for (auto &board : boards) {
        if (board.is_available())
            return &board;
    }
    return nullptr;
}


void Daemon::close_session(Session *session)
{
    wait_queue.remove_if([&](auto &s) { return s == session; });
    sessions.remove_if([&](auto &s) { return &s == session; });

    if (wait_queue.size() > 0) {
        Board *board = find_available_board();
        if (board) {
            Session *sess = wait_queue.front();
            wait_queue.pop_front();
            sess->assign_board(board);
        }
    }

    if (client_listener.is_from_systemd && sessions.empty()) {
        logger->info("No session active");
        loop.break_loop(); // exit
    }
}

void Daemon::print_status(int fd)
{
    for (const Session &sess: sessions) {
        if (sess.get_status() != Session::status::created) {
            string str = sess.get_status_line();
            dprintf(fd, "%s\n", str.c_str());
        }
    }
}

void Daemon::on_signal(ev::sig &w, int revents)
{
    logger->info("Received {} signal", strsignal(w.signum));
    loop.break_loop(ev::ALL);
}
