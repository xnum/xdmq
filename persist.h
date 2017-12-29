#pragma once
#include "raft.h"

void persist_init(const char*, int);
void set_term(int);
void set_vote(int);
int get_term();
int get_vote();
void persist_entry(msg_entry_t*);
void set_committed_index(int idx);
void persist_load_entries(raft_server_t *);
