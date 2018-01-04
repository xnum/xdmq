#pragma once

#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "raft.h"
#include "logger.h"
#include "buffer.h"
#include "queue.h"

#define member_size(type, member) sizeof(((type *)0)->member)

typedef struct produce_s {
    uv_tcp_t conn;
    QUEUE msg_head;
    QUEUE wait_queue;
} produce_t;

typedef struct p_entry_s {
    msg_entry_t req;
    msg_entry_response_t resp;
    QUEUE msg_queue;
} p_entry_t;

typedef int(*prod_read_cb)(uv_tcp_t *, const char* addr, int len);
typedef int(*prod_conn_cb)(produce_t *);
typedef int(*prod_disconn_cb)(produce_t *);



int produce_init(int port);
void produce_set_read_callback(prod_read_cb);
void produce_set_connected_callback(prod_conn_cb);
void produce_set_disconnected_callback(prod_disconn_cb);
void produce_response(produce_t *, p_entry_t *, int);
