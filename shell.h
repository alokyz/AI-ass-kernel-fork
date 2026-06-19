#ifndef ORANGEX_SHELL_H
#define ORANGEX_SHELL_H

#include "types.h"

#define SHELL_MAX_INPUT  1024
#define SHELL_MAX_ARGS   64
#define SHELL_HISTORY    50

void shell_init(void);
void shell_run(void);

#endif
