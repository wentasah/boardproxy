#include "boards.hpp"

std::list<Board> boards = {
#if defined(LOCAL_TEST)
  // Version for testing without real board (until we add configuration file support)
  { "bash -c 'echo Welcome to board 1; sleep 3'", "127.0.0.1" },
  { "bash -c 'echo Welcome to board 2; sleep 3'", "127.0.0.1" },
#else
#define CMD "ssh psr-hw@c2c1 sterm -b 2000 -s 115200 /dev/serial/by-id/"
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00EM7L-if00-port0", "10.35.95.50" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00ENK1-if00-port0", "10.35.95.51" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00EO5X-if00-port0", "10.35.95.52" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00EQSO-if00-port0", "10.35.95.53" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00EREM-if00-port0", "10.35.95.54" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00ERUW-if00-port0", "10.35.95.55" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DJ00F380-if00-port0", "10.35.95.56" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DN00JRK2-if00-port0", "10.35.95.57" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DN00JU72-if00-port0", "10.35.95.58" },
  { CMD "usb-FTDI_FT230X_Basic_UART_DN00JU7G-if00-port0", "10.35.95.59" },
#endif
};
