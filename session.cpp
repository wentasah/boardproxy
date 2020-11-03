#include <boost/asio.hpp>
#include <unistd.h>
#include <functional>
#include <boost/process/extend.hpp>
#include "session.hpp"
#include "debug.hpp"
#include "daemon.hpp"
#include "log.hpp"

using boost::asio::local::stream_protocol;
namespace bp = boost::process;

Session::Session(boost::asio::io_context &io, Daemon &daemon, stream_protocol::socket sock)
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
    boost::asio::async_read(sock, boost::asio::buffer(&header, sizeof(header)),
                     [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec != boost::asio::error::eof)
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
    using namespace std::placeholders;  // for _1, _2, _3...

    logger->info("Parent pid {}", getpid());
    child = bp::child(
                "sleep 1",
                io,             // Pass io_conext to allow async callbacks, i.e., on_exit
                bp::extend::on_exec_setup([](auto&) {
                    logger->info("From pid {}", getpid());
                }),
                bp::extend::on_error([](auto&, const std::error_code & ec) {
                    logger->error("Process spawn error: {}", ec.message());
                }),
                bp::on_exit(std::bind(&Session::on_process_exit, this, _1, _2))
    );
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

void Session::on_process_exit(int exit, const std::error_code &ec_in)
{
    logger->info("process exits: {}, {}", exit, ec_in.message());
    close_session();
}

void Session::close_session()
{
    if (child.running())
        child.terminate();
    daemon.close_session(this);
}
