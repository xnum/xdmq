#pragma once
#include "raft.h"
#include "msg.h"

#define member_size(type, member) sizeof(((type *)0)->member)

typedef struct entry_s {
    int term;
    int id;
    int type;
    int len;
    char text[MSG_SIZE];
} entry_t;


entry_t* get_entry(int n);
void persist_init(const char*, int);
void set_term(int);
void set_vote(int);
int get_term();
int get_vote();
void persist_entry(msg_entry_t*);
void set_committed_index(int idx);
void persist_load(raft_server_t *);
