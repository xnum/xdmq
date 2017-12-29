#include "persist.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include "logger.h"

typedef struct entry_s {
    int term;
    int id;
    int type;
    int len;
    char data[128];
} entry_t;

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

void persist_init(const char* ident, int n)
{
    int size = sizeof(persist_t) + 10000;
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

    slogf(INFO, "set=%d\nterm=%d\nvote=%d\ncommit_index=%d\nn_entry=%d\n", 
            data->set,
            data->term,
            data->vote,
            data->cmt_idx,
            data->n_entry
            );
}

void set_term(int t)
{
    data->set = 1;
    data->term = t;
    slogf(INFO, "set term = %d\n", data->term);
}

void set_vote(int v)
{
    data->set = 1;
    data->vote = v;
    slogf(INFO, "set vote = %d\n", data->vote);
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
    disk->term = entry->term;
    disk->id = entry->id;
    disk->type = entry->type;
    strncpy(disk->data, entry->data.buf, 128);
    data->n_entry++;
}

void set_committed_index(int idx)
{
    data->cmt_idx = idx;
}

void persist_load_entries(raft_server_t *raft)
{
    for(int i = 0; i < data->n_entry; ++i) {
        msg_entry_t ent;
        entry_t *d = &data->entry[i];
        ent.term = d->term;
        ent.id = d->id;
        ent.type = d->type;
        ent.data.buf = d->data;
        ent.data.len = strlen(d->data);
        raft_append_entry(raft, &ent);
    }
}
