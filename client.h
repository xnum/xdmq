#pragma once
#include <uv.h>
#include <stdlib.h>
#include <stddef.h>
#include <msgpack.h>
#include "logger.h"
#include "buffer.h"

#define IP_STR_LEN 255


typedef struct cli_ctx_s {
    char host[IP_STR_LEN];
    int port;
    uv_tcp_t client;
    uv_connect_t conn;
    buffer_t buff;
    int node_id;

    read_cb r_cb;
    msgpack_unpacker *unp;
} cli_ctx_t;

int cli_init(cli_ctx_t*);
