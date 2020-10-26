#include <asio.hpp>
#include "session.hpp"
#include <unistd.h>
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using asio::local::stream_protocol;
namespace bp = boost::process;

Session::Session(asio::io_context &io, Daemon &daemon, stream_protocol::socket sock)
    : daemon(daemon)
    , io(io)
    , sock(std::move(sock))
{
    logger->info("New session {}", (void*)this);
    start_reading_from_client();
    start_process();
}

Session::~Session()
{
    logger->info("Closing session {}", (void*)this);
}

void Session::start_reading_from_client()
{
    asio::async_read(sock, asio::buffer(&header, sizeof(header)),
                     [this](const asio::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec != asio::error::eof)
                logger->error("Read error: {}", ec.message());
            close_session();
        } else {
            switch (header.id) {
            case proto::msg_type::setup:
                logger->info("Setup");
                break;
            }
            start_reading_from_client();
        }
    });
}

void Session::start_process()
{
    child = bp::child("sleep 1",
                      bp::on_exit=[](int exit, const std::error_code& ec_in) {
        logger->info("bash exits: {}, {}", exit, ec_in.message());
    });
//    int pin[2], pout[2], perr[2];

//    CHECK(pipe(pin));
//    CHECK(pipe(pout));
//    CHECK(pipe(perr));

//    pid_t pid = CHECK(fork());
//    if (pid == 0) {
//        CHECK(dup2(pin[0], STDIN_FILENO));
//        CHECK(dup2(pout[1], STDOUT_FILENO));
//        CHECK(dup2(perr[1], STDERR_FILENO));
//        close(pin[0]);
//        close(pin[1]);
//        close(pout[0]);
//        close(pout[1]);
//        close(perr[0]);
//        close(perr[1]);
//        CHECK(execlp("bash", "bash", "-i", (char *) NULL));
//    }
//    close(pin[0]);
//    close(pout[1]);
//    close(perr[1]);

//    io_stdin.set(pin[1], ev::NONE);
//    io_stdout.start(pout[0], ev::READ);
//    io_stderr.start(perr[0], ev::READ);

//    child.set<Session, &Session::on_child>(this);
    //    child.start(pid);
}

void Session::close_session()
{
    daemon.close_session(this);
}
