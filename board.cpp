#include "board.hpp"

Board::Board(std::string id, std::string command, std::string ip_address, std::string close_command)

    : id(id)
    , command(command)
    , ip_address(ip_address)
    , close_command(close_command)
{

}
