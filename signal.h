#ifndef ORANGEX_SIGNAL_H
#define ORANGEX_SIGNAL_H

#include "types.h"

#define SIGNULL   0
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17
#define SIGSTOP   19
#define SIGCONT   18

#define SIG_DFL   0
#define SIG_IGN   1

typedef void (*signal_handler_t)(int sig);

void signal_init(void);
int  signal_send(uint32_t pid, int sig);
void signal_handle(int sig);
void signal_default_action(int sig);
const char* signal_name(int sig);

#endif
