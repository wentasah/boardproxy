#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <sys/types.h>

namespace proto {

enum class msg_type {
    setup
};

struct header {
    msg_type id;
    uint32_t length; // length without this header

    header() {}
    header(msg_type id, uint32_t length) : id(id), length(length) {}
};

struct setup : header {
    pid_t ppid;

    setup(pid_t ppid) : header(msg_type::setup, sizeof(*this)), ppid(ppid) {}
};

}
#endif
