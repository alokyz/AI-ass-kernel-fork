#ifndef ORANGEX_SYSCALL_H
#define ORANGEX_SYSCALL_H

#include "types.h"

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_GETPID  3
#define SYS_SLEEP   4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_REBOOT  8

#define SYSCALL_COUNT 9

void syscall_init(void);

#endif
