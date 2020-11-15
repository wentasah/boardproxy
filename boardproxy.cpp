#include <argp.h>
#include <ev++.h>
#include "daemon.hpp"
#include "client.hpp"
#include "log.hpp"
#include <spdlog/cfg/env.h>
#include "util.hpp"
#include "config.hpp"

using namespace std;

struct {
    string config;
    bool daemon = false;
    string name;
    string sock_dir;
    bool list_sessions = false;
} opt;

static error_t parse_opt(int key, char *arg, struct argp_state *argp_state)
{
    switch (key) {
    case 'c':
        opt.config = arg;
        break;
    case 'd':
        opt.daemon = true;
        break;
    case 'l':
        opt.list_sessions = true;
        break;
    case 'n':
        opt.name = arg;
        break;
    case 's':
        opt.sock_dir = arg;
        break;
    case ARGP_KEY_ARG:
        if (argp_state->arg_num == 0)
            opt.sock_dir = arg;
        else
            return ARGP_ERR_UNKNOWN;
        break;
    case ARGP_KEY_END:
        if (opt.daemon && !opt.name.empty())
            argp_error(argp_state, "--name is not allowed with --daemon");
        if (opt.daemon && opt.list_sessions)
            argp_error(argp_state, "--list-sessions is not allowed with --daemon");
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
    { "config",        'c', "FILE",      0,                   "Configuration file" },
    { "daemon",        'd', 0,           0,                   "Run as central daemon" },
    { "name",          'n', "NAME",      0,                   "Client username (useful if multiple users share one UNIX account)" },
    { "sock-dir",      's', "DIR",       OPTION_HIDDEN,       "Directory, where to create UNIX sockets (deprecated)" },
    { "list-sessions", 'l',  0,          0,                   "List all sessions" },
    { 0 }
};

/* Our argp parser. */
static struct argp argp = {
    options, parse_opt, "[SOCK_DIR]",

    "PSR boardproxy ... TBD"
};

int main(int argc, char *argv[])
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    ev::default_loop loop;

    spdlog::cfg::load_env_levels();
    spdlog::set_automatic_registration(false);

    try {
        Config cfg(opt.config);

        string sock_dir = !opt.sock_dir.empty() ? opt.sock_dir : cfg.sock_dir;
        if (sock_dir.empty())
            throw runtime_error("sock_dir not specified. Specify it either on command line or in the config file.");

        if (opt.daemon) {
            Daemon d(loop, sock_dir, move(cfg.boards));
            loop.run();
        } else {
            proto::setup::command_t command = proto::setup::command::connect;
            if (opt.list_sessions)
                command = proto::setup::command::list_sessions;
            Client c(loop, sock_dir, command, opt.name);
            loop.run();
        }
    }  catch (std::exception &e) {
        logger->error(e.what());
        return 1;
    }

    return 0;
}
