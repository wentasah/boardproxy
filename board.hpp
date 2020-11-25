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

    Board(std::string id, std::string command, std::string ip_address);

    bool is_available() { return owner == nullptr; }

    void acquire(Session *session) { owner = session; }
    void release() { owner = nullptr; }
private:

    Session *owner = nullptr;
};

#endif // BOARD_HPP
