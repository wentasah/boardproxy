#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <sys/types.h>
#include <string>
#include <cstring>

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

// Setup message sent from client to daemon when the client starts. In
// addition to the data fields seen below, the client also sends with
// this message ancillary data with client's stdin/out/err file
// descriptors.
struct setup : header {
    pid_t ppid;
    char username[32] = { 0 };

    setup(pid_t ppid,
          const std::string user)
        : header(msg_type::setup, sizeof(*this))
        , ppid(ppid)
    {
        strncpy(username, user.c_str(), sizeof(username) - 1);
    }
};

}
#endif
