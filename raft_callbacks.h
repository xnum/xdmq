#pragma once

#include <msgpack.h>
#include "raft.h"
#include "raft_ext.h"
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


int applylog(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety
        );

int persist_term(
        raft_server_t* raft,
        void *udata,
        const int current_term
        );

int persist_vote(
        raft_server_t* raft,
        void *udata,
        const int voted_for
        );

int logentry_offer(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety,
        int ety_idx
        );

int logentry_poll(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *entry,
        int ety_idx
        );

int logentry_pop(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *entry,
        int ety_idx
        );
