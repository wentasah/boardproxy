#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <list>
#include "board.hpp"

class Config {
public:
    std::string sock_dir;
    std::list<Board> boards;

    Config(std::string filename);
};
#endif // CONFIG_HPP
