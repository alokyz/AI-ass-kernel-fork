#include "pipe.h"
#include "heap.h"
#include "string.h"

pipe_t* pipe_create(void) {
    pipe_t* p = (pipe_t*)kmalloc(sizeof(pipe_t));
    if (!p) return NULL;
    p->read_pos = 0;
    p->write_pos = 0;
    p->count = 0;
    p->readers = 1;
    p->writers = 1;
    p->closed = 0;
    return p;
}

void pipe_destroy(pipe_t* pipe) {
    if (pipe) { pipe->closed = 1; kfree(pipe); }
}

int pipe_write(pipe_t* pipe, const char* data, uint32_t len) {
    if (!pipe || pipe->closed) return -1;
    uint32_t i;
    for (i = 0; i < len; i++) {
        if (pipe->count >= PIPE_BUF_SIZE) break;
        pipe->buffer[pipe->write_pos] = data[i];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUF_SIZE;
        pipe->count++;
    }
    return (int)i;
}

int pipe_read(pipe_t* pipe, char* data, uint32_t len) {
    if (!pipe || pipe->closed) return -1;
    uint32_t i = 0;
    while (i < len && pipe->count > 0) {
        data[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUF_SIZE;
        pipe->count--;
        i++;
    }
    return (int)i;
}

int pipe_data_available(pipe_t* pipe) {
    return pipe ? (int)pipe->count : 0;
}
