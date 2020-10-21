#include <asio.hpp>
#include "client.hpp"
#include <unistd.h>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using asio::local::stream_protocol;

Client::Client(asio::io_context &io, Daemon &daemon, stream_protocol::socket sock)
    : daemon(daemon)
    , sock(std::move(sock))
{


    //io_sock.set(sock_fd, ev::READ);
    //io_sock.set<Client, &Client::on_sock>(this);

    int pin[2], pout[2], perr[2];

    CHECK(pipe(pin));
    CHECK(pipe(pout));
    CHECK(pipe(perr));

    pid_t pid = CHECK(fork());
    if (pid == 0) {
        CHECK(dup2(pin[0], STDIN_FILENO));
        CHECK(dup2(pout[1], STDOUT_FILENO));
        CHECK(dup2(perr[1], STDERR_FILENO));
        close(pin[0]);
        close(pin[1]);
        close(pout[0]);
        close(pout[1]);
        close(perr[0]);
        close(perr[1]);
        CHECK(execlp("bash", "bash", "-i", (char *) NULL));
    }
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);

    io_stdin.set(pin[1], ev::NONE);
    io_stdout.start(pout[0], ev::READ);
    io_stderr.start(perr[0], ev::READ);

    child.set<Client, &Client::on_child>(this);
    child.start(pid);
}

void Client::start_reading_from_client()
{
    asio::async_read(sock, asio::buffer(&header, sizeof(header)),
                     [this](const asio::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            logger->error("Read error {}", ec);
        } else {

        }

    }
    sock.async_read_some()
}

void Client::on_sock(ev::io &w, int revents)
{

}

void Client::on_child(ev::child &w, int revents)
{

}
