#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>

namespace proto {

enum class msg_type {
    stdin = 1,
    stdout,
    stderr,
};

struct msg_header {
    msg_type id;
    uint32_t length; // length without this header
};

}
#endif