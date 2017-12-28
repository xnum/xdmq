#include "raft_callbacks.h"


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

    int e = uv_try_write(&conn->client, bufs, 1);
    if (e < 0)
        slogf(ERR, "%s\n", uv_strerror(e));

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

    // TODO log?
    /* let us ignore log */
    for(int i = 0; i < m->n_entries; ++i) {
        msgpack_pack_bin(&pk, sizeof(msg_entry_t));
        msgpack_pack_bin_body(&pk, &m->entries[i], sizeof(msg_entry_t));
        //msgpack_pack_int(&pk, m->entries[i].data.len);
        msgpack_pack_bin(&pk, m->entries[i].data.len);
        msgpack_pack_bin_body(&pk, m->entries[i].data.buf, m->entries[i].data.len);
    }

    uv_buf_t bufs[1];
    bufs[0] = uv_buf_init(sbuf.data, sbuf.size);

    int e = uv_try_write(&conn->client, bufs, 1);
    if (e < 0)
        slogf(ERR, "%s\n", uv_strerror(e));

    msgpack_sbuffer_destroy(&sbuf);
    return 0;

}

int applylog(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety
        )
{
    //slogf(INFO, "applying log %s\n", ety->data.buf);
    return 0;
}

int persist_term(
        raft_server_t* raft,
        void *udata,
        const int current_term
        )
{
    return 0;
}

int persist_vote(
        raft_server_t* raft,
        void *udata,
        const int voted_for
        )
{
    return 0;
}

int logentry_offer(
        raft_server_t* raft,
        void *udata,
        raft_entry_t *ety,
        int ety_idx
        )
{
    slogf(INFO, "offer log %s\n", ety->data.buf);
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
