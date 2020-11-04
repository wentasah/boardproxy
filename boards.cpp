#include "boards.hpp"

std::list<Board> boards = {
#if defined(LOCAL_TEST)
  // Version for testing without real board (until we add configuration file support)
  { "bash -c 'echo Welcome to board 1; sleep 3'", "127.0.0.1" },
  { "bash -c 'echo Welcome to board 2; sleep 3'", "127.0.0.1" },
#else
#endif
};
