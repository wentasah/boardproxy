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
#ifndef BOARD_HPP
#define BOARD_HPP

#include <string>

class Session;

class Board
{
public:
    const std::string id;
    const std::string command;
    const std::string ip_address;
    const std::string close_command;
    const std::string reserved_for;

    Board(std::string id, std::string command, std::string ip_address,
          std::string close_command, std::string reserved_for);

    bool is_available() const;

    void acquire(Session *session) { owner = session; }
    void release() { owner = nullptr; }
private:

    Session *owner = nullptr;
};

#endif // BOARD_HPP
