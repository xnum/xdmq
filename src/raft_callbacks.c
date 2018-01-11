#include "raft_callbacks.h"
#include "persist.h"


#define RAFT_BUFLEN 512

int send_requestvote(
        raft_server_t* raft,
        void *udata,
        raft_node_t* node,
        msg_requestvote_t* m
        )
{
    cli_ctx_t* conn = raft_node_get_udata(node);

    /* serialize */ 
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_int(&pk, MSG_REQUESTVOTE);
    msgpack_pack_bin(&pk, sizeof(msg_requestvote_t));
    msgpack_pack_bin_body(&pk, m, sizeof(msg_requestvote_t));

    uv_buf_t bufs[1];
    bufs[0] = uv_buf_init(sbuf.data, sbuf.size);

    if(conn->status == 1)
        uv_try_write(&conn->client, bufs, 1);

    msgpack_sbuffer_destroy(&sbuf);
    return 0;
}

int send_appendentries(
        raft_server_t* raft,
        void *user_data,
        raft_node_t* node,
        msg_appendentries_t* m
        )
{
    cli_ctx_t* conn = raft_node_get_udata(node);

    /* serialize */ 
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_int(&pk, MSG_APPENDENTRIES);
    msgpack_pack_bin(&pk, sizeof(msg_appendentries_t));
    msgpack_pack_bin_body(&pk, m, sizeof(msg_appendentries_t));

    for(int i = 0; i < m->n_entries; ++i) {
        msgpack_pack_bin(&pk, sizeof(msg_entry_t));
        msgpack_pack_bin_body(&pk, &m->entries[i], sizeof(msg_entry_t));
        msgpack_pack_bin(&pk, m->entries[i].data.len);
        msgpack_pack_bin_body(&pk, m->entries[i].data.buf, m->entries[i].data.len);
    }

    uv_buf_t bufs[1];
    bufs[0] = uv_buf_init(sbuf.data, sbuf.size);

    if(conn->status == 1)
        uv_try_write(&conn->client, bufs, 1);

    msgpack_sbuffer_destroy(&sbuf);
    return 0;

}

int applylog(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety
        )
{
    static FILE* f = NULL;
    if(!f) {
        char tmp[128] = {};
        snprintf(tmp, 128, "xdmq.out.%d", (int) udata);
        f = fopen(tmp, "a+");
    }

    int len = ety->data.len;
    //slogf(INFO, "applying log %d %*.*s\n", len, len, len, ety->data.buf);
    assert(f != NULL);
    fprintf(f, "%*.*s\n", 32, 32, ety->data.buf);
    fflush(f);
    set_committed_index(raft_get_commit_idx(raft));
    return 0;
}

int persist_term(
        raft_server_t* raft,
        void *udata,
        const int current_term
        )
{
    slogf(INFO, "persist_term %d\n", current_term);
    set_term(current_term);
    return 0;
}

int persist_vote(
        raft_server_t* raft,
        void *udata,
        const int voted_for
        )
{
    set_vote(voted_for);
    return 0;
}

int logentry_offer(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety,
        int ety_idx
        )
{
    int len = ety->data.len;
    /*
    slogf(INFO, "offer log %*.*s\n", len, len, ety->data.buf);
    slogf(INFO, "now = %d\tcmt_idx = %d\tlast applied = %d\t", 
            ety_idx,
            raft_get_commit_idx(raft),
            raft_get_last_applied_idx(raft));
            */
    persist_entry(ety);
    return 0;
}

int logentry_poll(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *entry,
        int ety_idx
        )
{
    return 0;
}

int logentry_pop(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *entry,
        int ety_idx
        )
{
    return 0;
}
