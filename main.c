#include <assert.h>
#include <msgpack.h>
#include "raft.h"
#include "server.h"
#include "client.h"

#define RAFT_BUFLEN 512

typedef enum
{
    /** Handshake is a special non-raft message type
     *      * We send a handshake so that we can identify ourselves to our peers */
    MSG_HANDSHAKE,
    /** Successful responses mean we can start the Raft periodic callback */
    MSG_HANDSHAKE_RESPONSE,
    /** Tell leader we want to leave the cluster */
    /* When instance is ctrl-c'd we have to gracefuly disconnect */
    MSG_LEAVE,
    /* Receiving a leave response means we can shutdown */
    MSG_LEAVE_RESPONSE,
    MSG_REQUESTVOTE,
    MSG_REQUESTVOTE_RESPONSE,
    MSG_APPENDENTRIES,
    MSG_APPENDENTRIES_RESPONSE,
} peer_message_type_e;

raft_server_t *raft = NULL;
cli_ctx_t ctxs[5];

static int send_requestvote(
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

static int send_appendentries(
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

int read_pac(const char* addr, int len)
{
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    int id = atoi(argv[1]);
    slogf(INFO, "id = %d\n", id);

    raft = raft_new();
    for(int i = 0; i < 4; ++i)
        raft_add_node(raft, &ctxs[i], i, id == i);

    raft_cbs_t raft_callbacks = {
        .send_requestvote            = send_requestvote,
        .send_appendentries          = send_appendentries,
        .applylog                    = applylog,
        .persist_vote                = persist_vote,
        .persist_term                = persist_term,
        .log_offer                   = raft_logentry_offer,
        .log_poll                    = raft_logentry_poll,
        .log_pop                     = raft_logentry_pop,
        .log                         = raft_log,
    };

    char* user_data = "test";

    raft_set_callbacks(raft, &raft_callbacks, user_data);


    /* TCP */

    serv_init(9000 + id, read_pac);

    sleep(1);

    for(int i = 0; i < 4; ++i)
        if(i != id) {
            strncpy(ctxs[i].host, "127.0.0.1", IP_STR_LEN);
            ctxs[i].port = 9000+i;
            ctxs[i].buff = buffer_init();
            ctxs[i].r_cb = read_pac;
            cli_init(&ctxs[i]);
        }

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
