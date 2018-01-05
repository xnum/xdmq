#include "persist.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include "logger.h"

typedef struct persist_s {
    int set;
    int term;
    int vote;
    int cmt_idx;
    int n_entry;
    entry_t entry[];
} persist_t;

static int fd;
static persist_t *data;
static const int size = sizeof(persist_t) + 1024 * 1024 * sizeof(entry_t); /* 48B */

void persist_init(const char* ident, int n)
{
    char tmp[256] = {};
    snprintf(tmp, 256, "%s.%d", ident, n);

    fd = open(tmp, O_RDWR | O_CREAT, 0664);

    struct stat st;
    fstat(fd, &st);
    if(st.st_size != size) {
        slogf(WARN, "truncated\n");
        ftruncate(fd, size);
    }

    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 

    /*
    slogf(INFO, "set=%d\nterm=%d\nvote=%d\ncommit_index=%d\nn_entry=%d\n", 
            data->set,
            data->term,
            data->vote,
            data->cmt_idx,
            data->n_entry
            );
            */

    slogf(INFO, "Total available entries # = %d\n", (size-sizeof(persist_t)) / sizeof(entry_t));
}

entry_t* get_entry(int n)
{
    return &data->entry[n];
}

void set_term(int t)
{
    data->set = 1;
    data->term = t;
    //slogf(INFO, "set term = %d\n", data->term);
}

void set_vote(int v)
{
    data->set = 1;
    data->vote = v;
    //slogf(INFO, "set vote = %d\n", data->vote);
}

int get_term()
{
    if(data->set)
        return data->term;
    else
        return -1;
}

int get_vote()
{
    if(data->set)
        return data->vote;
    else
        return -1;
}

int get_cmt_idx()
{
    if(data->set)
        return data->cmt_idx;
    else
        return -1;
}

void persist_entry(msg_entry_t *entry)
{
    int idx = data->n_entry;
    entry_t *disk = &data->entry[idx];
    if((void*)disk - (void*)data >= size) {
        slogf(CRIT, "memory overflow\n");
    }

    disk->term = entry->term;
    disk->id = entry->id;
    disk->type = entry->type;
    memcpy(disk->data, entry->data.buf, entry->data.len);
    free(entry->data.buf);
    entry->data.buf = disk->data;
    data->n_entry++;
}

void set_committed_index(int idx)
{
    data->cmt_idx = idx;
}

// load order is very important
void persist_load(raft_server_t *raft)
{
    if(!data->set)return;

    for(int i = 0; i < data->n_entry; ++i) {
        msg_entry_t ent;
        entry_t *d = &data->entry[i];
        ent.term = d->term;
        ent.id = d->id;
        ent.type = d->type;
        ent.data.len = strlen(d->data);
        ent.data.buf = strdup(d->data); // prevent double-freed at applylog()
        raft_append_entry(raft, &ent);
    }

    raft_set_commit_idx(raft, data->cmt_idx);
    raft_apply_all(raft);

    raft_set_current_term(raft, data->term);
    raft_vote_for_nodeid(raft, data->vote);
}
