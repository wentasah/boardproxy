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

static string get_board_field(const string key, const string templ, const toml::table &board)
{
    string result;

    auto field_node = board[key];

    if (field_node.type() == toml::node_type::none) {
        // No explicit field for the board - use global field template
        fmt::dynamic_format_arg_store<fmt::format_context> args;
        for (auto [k, v] : board) {
            if (!v.is_string())
                throw runtime_error("value for " + k + " is not string (at " + to_string(v.source().begin) + ")");
            args.push_back(fmt::arg(k.c_str(), v.value_or("")));
        }
        result = fmt::vformat(templ, args);
    } else {
        // Board has explicit field
        if (!field_node.is_string())
            throw runtime_error(key + " is not a string at " + to_string(board.source()));
        result = field_node.value_or("");
    }
    return result;
}

static Board create_board(
        const string &id,
        const string &command_template,
        const string &close_command_template,
        const toml::table &board)
{
    const auto ip_address = board["ip_address"].value<string>();
    if (!ip_address)
        throw runtime_error("no or invalid ip_address for board at " + to_string(board.source().begin));

    return Board(id,
                 get_board_field("command", command_template, board),
                 *ip_address,
                 get_board_field("close_command", close_command_template, board));
}

Config::Config(string filename)
{
    if (filename.empty())
        return;

    try {
        const toml::table cfg = toml::parse_file(filename);

        const auto command_template = cfg["command_template"].value_or(""s);
        const auto close_command_template = cfg["close_command_template"].value_or(""s);

        auto sd = cfg["sock_dir"].value<string>();
        if (sd)
            sock_dir = *sd;

        const auto boards = cfg["boards"].as_table();
        if (!boards)
            throw runtime_error(filename + ": boards is not TOML table");

        for (auto [key, val] : *boards) {
            const auto board = val.as_table();
            if (!board)
                throw runtime_error(filename + ": boards." + key + " is not TOML table (at " + to_string(board->source().begin) + ")");

            this->boards.push_back(
                create_board(
                    key,
                    command_template,
                    close_command_template,
                    *board));
        }
    }
    catch (const toml::parse_error& err) {
        stringstream msg;
        msg << "Error parsing file '" << *err.source().path << "': "
            << err.description() << " (" << err.source().begin << ")";
        throw runtime_error(msg.str());
    }

}
