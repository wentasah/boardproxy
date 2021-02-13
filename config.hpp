#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <list>
#include <memory>
#include "board.hpp"
#include "proxy_factory.hpp"

class Config {
public:
    std::string sock_dir;
    std::list<Board> boards;
    std::list<std::unique_ptr<ProxyFactory>> proxy_factories;

    Config(std::string filename);
};
#endif // CONFIG_HPP
