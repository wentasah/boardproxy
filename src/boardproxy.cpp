// Copyright (C) 2021 Michal Sojka <michal.sojka@cvut.cz>
// 
// This file is part of boardproxy.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <argp.h>
#include <ev++.h>
#include <err.h>
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
    bool allow_set_authorized_keys = false;
    bool no_wait = false;
} opt;

static int handle_ssh_command(const string command)
{
    if (command == "set-authorized-keys") {
        int ret = system("umask 077 && cd && mkdir -m 700 -p .ssh && "
                         "f=$(mktemp .ssh/authorized_keys.XXXXXX); cat > $f && "
                         "mv --backup=numbered $f .ssh/authorized_keys");
        if (ret == -1)
            err(1, "set-keys");
        return ret;
    } else {
        warnx("Unsupported command: %s", command.c_str());
        return 1;
    }
}

enum {
    OPT_ALLOW_SET_AUTHORIZED_KEYS = 1000,
    OPT_NO_WAIT,
};

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
    case OPT_ALLOW_SET_AUTHORIZED_KEYS:
        opt.allow_set_authorized_keys = true;
        break;
    case OPT_NO_WAIT:
        opt.no_wait = true;
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
        if (opt.allow_set_authorized_keys && opt.daemon)
            argp_error(argp_state, "--allow_set_authorized_keys is not allowed with --daemon");
        if (opt.daemon && opt.no_wait)
            argp_error(argp_state, "--no-wait is not allowed with --daemon");
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
    { .doc = "Daemon options:" },
    { "config",        'c', "FILE",      0,                   "Configuration file" },
    { "daemon",        'd', 0,           0,                   "Run as central daemon. Without this option, the program runs as a client connecting to the daemon." },

    { .doc = "Client options:" },
    { "allow-set-authorized-keys", OPT_ALLOW_SET_AUTHORIZED_KEYS, 0, 0,
      "Allow set-authorized-keys subcommand to (re)write ~/.ssh/authorized_keys file" },
    { "name",          'n', "NAME",      0,                   "Client username (useful if multiple users share one UNIX account)" },
    { "sock-dir",      's', "DIR",       OPTION_HIDDEN,       "Directory, where to create UNIX sockets (deprecated)" },
    { "list-sessions", 'l',  0,          0,                   "List all sessions" },
    { "no-wait",       OPT_NO_WAIT,  0,  0,                   "Don't wait if no board is available" },
    { 0 }
};

/* Our argp parser. */
static struct argp argp = {
    options, parse_opt, "[SOCK_DIR]",

    "Access a pool of remote boards (or other resources) with "
    "connection proxying via SSH port forwarding."
};

int main(int argc, char *argv[])
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    ev::default_loop loop;

    spdlog::cfg::load_env_levels();
    spdlog::set_automatic_registration(false);

    if (opt.allow_set_authorized_keys) {
        const char *ssh_original_command = getenv("SSH_ORIGINAL_COMMAND");
        if (ssh_original_command)
            return handle_ssh_command(ssh_original_command);
    }

    try {
        Config cfg(opt.config);

        string sock_dir = !opt.sock_dir.empty() ? opt.sock_dir : cfg.sock_dir;
        if (sock_dir.empty())
            throw runtime_error("sock_dir not specified. Specify it either on command line or in the config file.");

        if (opt.daemon) {
            Daemon d(loop, sock_dir, move(cfg.boards), move(cfg.proxy_factories));
            loop.run();
        } else {
            proto::setup::command_t command = proto::setup::command::connect;
            if (opt.list_sessions)
                command = proto::setup::command::list_sessions;
            Client c(loop, sock_dir, command, opt.name, opt.no_wait);
            loop.run();
        }
    }  catch (std::exception &e) {
        logger->error(e.what());
        return 1;
    }

    return 0;
}
