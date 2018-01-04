#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uv.h>

#include "logger.h"

#define _BUFFER_SIZE (65536<<1)

#define container_of(ptr, type, member) ({ \
                     const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                     (type *)( (char *)__mptr - offsetof(type,member) );})


struct buffer_s {
    int ava_size; 
    int max_size;
    void *data;
    char buf[_BUFFER_SIZE];
    char *begin;
};

typedef struct buffer_s* buffer_t;

buffer_t buffer_init();


int buffer_max_size(buffer_t t);

void buffer_clear(buffer_t t);

uint32_t buffer_size(buffer_t t);

char* buffer_begin(buffer_t t);

void buffer_consume(buffer_t t, int n);

void buffer_produced(buffer_t t, int written_size);

void buffer_append(buffer_t t, const char *buf, int len);

void buffer_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

#endif

