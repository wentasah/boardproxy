#include <argp.h>
#include <ev++.h>
#include "version.h"
#include "daemon.hpp"
#include "client.hpp"
#include "log.hpp"
#include <spdlog/cfg/env.h>
#include "util.hpp"

using namespace std;

struct {
    bool daemon = false;
    string name;
    string sock_dir = "/run/psr-hw";
} opt;

static error_t parse_opt(int key, char *arg, struct argp_state *argp_state)
{
    switch (key) {
    case 'd':
        opt.daemon = true;
        break;
    case 'n':
        opt.name = arg;
        break;
    case 's':
        opt.sock_dir = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
    { "daemon",   'd', 0,     0, "Run as central daemon" },
    { "name",     'n', "NAME",0, "Client username (useful if multiple users share one UNIX account)" },
    { "sock-dir", 's', "DIR", 0, "Directory, where to create UNIX sockets" },
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

    ev::default_loop loop;

    spdlog::cfg::load_env_levels();
    spdlog::set_automatic_registration(false);

    try {
        if (opt.daemon) {
            Daemon d(loop, opt.sock_dir);
            loop.run();
        } else {
            Client c(loop, opt.sock_dir, opt.name);
            loop.run();
        }
    }  catch (std::exception &e) {
        logger->error(e.what());
        return 1;
    }

    return 0;
}
