#pragma once

#include <msgpack.h>
#include "raft.h"
#include "client.h"

int send_requestvote(
        raft_server_t* raft,
        void *udata,
        raft_node_t* node,
        msg_requestvote_t* m
        );

int send_appendentries(
        raft_server_t* raft,
        void *user_data,
        raft_node_t* node,
        msg_appendentries_t* m
        );


