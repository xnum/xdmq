#pragma once
#include <uv.h>
#include <stdlib.h>
#include <stddef.h>
#include <msgpack.h>
#include "logger.h"
#include "server.h"
#include "produce.h"
#include "buffer.h"
#include "raft_ext.h"

#define IP_STR_LEN 255


typedef struct cli_ctx_s {
    char host[IP_STR_LEN];
    int port;
    uv_tcp_t client;
    uv_connect_t conn;
    buffer_t buff;
    int node_id;
    int self_id;

    read_cb r_cb;
    msgpack_unpacker *unp;

    int status;

    raft_node_t node;
} cli_ctx_t;

int cli_init(cli_ctx_t*);
