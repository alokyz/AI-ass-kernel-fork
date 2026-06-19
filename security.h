#ifndef ORANGEX_SECURITY_H
#define ORANGEX_SECURITY_H

#include "types.h"

#define SEC_OK          0
#define SEC_TAMPERED    1
#define SEC_UNTRUSTED   2
#define SEC_MALICIOUS   3

typedef struct {
    char     path[128];
    uint32_t checksum;
    uint32_t size;
    uint32_t flags;
} file_integrity_t;

void     security_init(void);
uint32_t security_hash(const uint8_t* data, uint32_t len);
int      security_check_file(const char* path);
void     security_scan(void);
void     security_add_trusted(const char* path);
int      security_is_trusted(const char* path);
void     security_sandbox_info(void);

#endif
