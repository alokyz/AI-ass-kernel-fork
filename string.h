#ifndef ORANGEX_STRING_H
#define ORANGEX_STRING_H

#include "types.h"

static inline size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static inline int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static inline int strncmp(const char* a, const char* b, size_t n) {
    size_t i;
    for (i = 0; i < n && a[i] && b[i]; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return (i < n) ? ((unsigned char)a[i] - (unsigned char)b[i]) : 0;
}

static inline char* strcpy(char* d, const char* s) {
    char* r = d;
    while ((*d++ = *s++));
    return r;
}

static inline void* memcpy(void* d, const void* s, size_t n) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;
    size_t i;
    for (i = 0; i < n; i++) dst[i] = src[i];
    return d;
}

static inline void* memmove(void* d, const void* s, size_t n) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;
    size_t i;
    if (dst < src) {
        for (i = 0; i < n; i++) dst[i] = src[i];
    } else {
        for (i = n; i > 0; i--) dst[i - 1] = src[i - 1];
    }
    return d;
}

static inline void* memset(void* d, int c, size_t n) {
    uint8_t* dst = (uint8_t*)d;
    size_t i;
    for (i = 0; i < n; i++) dst[i] = (uint8_t)c;
    return d;
}

#endif
