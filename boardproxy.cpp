#include <argp.h>
#include "version.h"
#include <asio/io_context.hpp>
#include "daemon.hpp"

using namespace std;

struct {
    bool daemon = false;
} opt;

static error_t parse_opt(int key, char *arg, struct argp_state *argp_state)
{
    switch (key) {
    case 'd':
        opt.daemon = true;
	    break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
    { "daemon", 'd', 0, 0, "Run as central daemon" },
    { 0 }
};

const char * argp_program_version = "boardproxy " GIT_VERSION;

/* Our argp parser. */
static struct argp argp = {
    options, parse_opt, 0,

    "PSR boardproxy ... TBD"
};

int main(int argc, char *argv[])
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    asio::io_context io;

    string dir("/run/psr-hw");

    if (opt.daemon) {
        Daemon d(io, dir);
        io.run();
    }

    return 0;
}