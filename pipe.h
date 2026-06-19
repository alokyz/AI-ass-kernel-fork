#ifndef ORANGEX_PIPE_H
#define ORANGEX_PIPE_H

#include "types.h"

#define PIPE_BUF_SIZE 4096

typedef struct {
    char     buffer[PIPE_BUF_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    int      readers;
    int      writers;
    int      closed;
} pipe_t;

pipe_t* pipe_create(void);
void    pipe_destroy(pipe_t* pipe);
int     pipe_write(pipe_t* pipe, const char* data, uint32_t len);
int     pipe_read(pipe_t* pipe, char* data, uint32_t len);
int     pipe_data_available(pipe_t* pipe);

#endif
