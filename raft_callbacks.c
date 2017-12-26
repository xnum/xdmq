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
    msgpack_pack_int(&pk, m->term);
    msgpack_pack_int(&pk, m->prev_log_idx);
    msgpack_pack_int(&pk, m->prev_log_term);
    msgpack_pack_int(&pk, m->leader_commit);
    msgpack_pack_int(&pk, m->n_entries);

    /* let us ignore log */

    uv_buf_t bufs[1];
    bufs[0] = uv_buf_init(sbuf.data, sbuf.size);

    int e = uv_try_write(&conn->client, bufs, 1);
    if (e < 0)
        slogf(ERR, "%s\n", uv_strerror(e));

    msgpack_sbuffer_destroy(&sbuf);
    return 0;

}

