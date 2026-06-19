#ifndef ORANGEX_DEVTREE_H
#define ORANGEX_DEVTREE_H

#include "types.h"

#define DEV_TTY     0x01
#define DEV_NULL    0x02
#define DEV_RANDOM  0x03
#define DEV_ZERO    0x04
#define DEV_KBD     0x05
#define DEV_VGA     0x06
#define DEV_SDA     0x07
#define DEV_NET     0x08

typedef struct dev_node {
    char     name[32];
    uint32_t type;
    uint32_t major;
    uint32_t minor;
    struct dev_node* next;
} dev_node_t;

void devtree_init(void);
dev_node_t* devtree_find(const char* name);
void devtree_list(void);

#endif
