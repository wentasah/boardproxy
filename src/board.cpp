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
#include "board.hpp"

Board::Board(std::string id, std::string command, std::string ip_address, std::string close_command, std::string reserved_for)

    : id(id)
    , command(command)
    , ip_address(ip_address)
    , close_command(close_command)
    , reserved_for(reserved_for)
{

}

bool Board::is_available() const {
    return owner == nullptr;
}
