#include <fstream> //required for parse_file()
#include <exception>
#include <sstream>
#include "config.hpp"
#include "toml.hpp"
#include <fmt/format.h>
#include <iostream>

using namespace std;

template <typename T>
string to_string( const T& value )
{
    ostringstream ss;
    ss << value;
    return ss.str();
}

static Board create_board(const string &id, const string &command_template, const toml::table &board)
{
    const auto ip_address = board["ip_address"].value<string>();
    if (!ip_address)
        throw runtime_error("no or invalid ip_address for board at " + to_string(board.source().begin));

    string command;

    auto command_node = board["command"];
    if (command_node.type() == toml::node_type::none) {
        // No explicit command for the board - use global command_template
        fmt::dynamic_format_arg_store<fmt::format_context> args;
        for (auto [k, v] : board) {
            if (!v.is_string())
                throw runtime_error("value for " + k + " is not string (at " + to_string(v.source().begin) + ")");
            args.push_back(fmt::arg(k.c_str(), v.value_or("")));
        }
        command = fmt::vformat(command_template, args);
    } else {
        // Board has explicit command
        if (!command_node.is_string())
            throw runtime_error("command is not a string at " + to_string(board.source()));
        command = command_node.value_or("");
    }

    return Board(id, command, *ip_address);
}

Config::Config(string filename)
{
    if (filename.empty())
        return;

    try {
        const toml::table cfg = toml::parse_file(filename);

        const auto command_template = cfg["command_template"].value<string>();

        auto sd = cfg["sock_dir"].value<string>();
        if (sd)
            sock_dir = *sd;

        const auto boards = cfg["boards"].as_table();
        if (!boards)
            throw runtime_error(filename + ": boards is not TOML table");

        for (auto [k, v] : *boards) {
            const auto board = v.as_table();
            if (!board)
                throw runtime_error(filename + ": boards." + k + " is not TOML table (at " + to_string(board->source().begin) + ")");

            this->boards.push_back(create_board(k, *command_template, *board));
        }
    }
    catch (const toml::parse_error& err) {
        stringstream msg;
        msg << "Error parsing file '" << *err.source().path << "': "
            << err.description() << " (" << err.source().begin << ")";
        throw runtime_error(msg.str());
    }

}
