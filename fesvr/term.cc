#include "term.h"
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

class canonical_termios_t
{
 public:
  canonical_termios_t()
   : restore_tios(false)
  {
    if (tcgetattr(0, &old_tios) == 0)
    {
      struct termios new_tios = old_tios;
      new_tios.c_lflag &= ~(ICANON | ECHO);
      if (tcsetattr(0, TCSANOW, &new_tios) == 0)
        restore_tios = true;
    }
  }

  ~canonical_termios_t()
  {
    if (restore_tios)
      tcsetattr(0, TCSANOW, &old_tios);
  }
 private:
  struct termios old_tios;
  bool restore_tios;
};

static canonical_termios_t tios; // exit() will clean up for us

//=====================
#include <fcntl.h>
#include <pty.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>

static int master_fd, slave_fd;
int init_pty() {
  char slave_name[256];
  if (openpty(&master_fd, &slave_fd, slave_name, NULL, NULL) == -1) {
    perror("openpty failed");
    exit(EXIT_FAILURE);
  }
  char  cmd[512]={0};
  sprintf(cmd, "xterm -hold -e bash -c \"screen %s; echo Press any key to exit; read\" &", slave_name);
  system(cmd);
  // set no-block flag
  int flags = fcntl(master_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
  printf("Slave PTY path: %s\nWaiting for any connection!\n", slave_name);
  fflush(stdout);
  // detect the client connects
  // TODO:bizarre behaviour! seems if no client like screen connected to consume
  // the character, reading the fd will get the character perviously written!
  while (true) {
    const char test = 0;
    char read_val = 0;
    write(master_fd, &test, 1);
    usleep(1000);
    if (read(master_fd, &read_val, 1) == -1)
      break;
  }
  printf("connected!\n");
  return 0;
}

void pty_write_byte(char c) {
  if (write(master_fd, &c, 1) != 1) {
    perror("write to pty failed");
  }
}

void pty_write_str(const char *str) {
  if (!str)
    return;
  write(master_fd, str, strlen(str));
}

int pty_read_byte(char* buf) {
  return read(master_fd, &buf, 1);
}
//======================
int canonical_terminal_t::read()
{
/*   struct pollfd pfd;
  pfd.fd = 0;
  pfd.events = POLLIN;
  int ret = poll(&pfd, 1, 0);
  if (ret <= 0 || !(pfd.revents & POLLIN))
    return -1;

  unsigned char ch;
  ret = ::read(0, &ch, 1);
  return ret <= 0 ? -1 : ch; */
  // struct pollfd pfd;
  // pfd.fd = 0;
  // pfd.events = POLLIN;
  // int ret = poll(&pfd, 1, 0);
  // if (ret <= 0 || !(pfd.revents & POLLIN))
  //   return -1;

  unsigned char ch;
  int ret = ::read(master_fd, &ch, 1);
  return ret <= 0 ? -1 : ch;
}

void canonical_terminal_t::write(char ch)
{
/*   if (::write(1, &ch, 1) != 1)
    abort(); */
    pty_write_byte(ch);
}
