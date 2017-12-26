#include <assert.h>
#include <msgpack.h>
#include "raft.h"
#include "server.h"
#include "client.h"
#include "raft_callbacks.h"


raft_server_t *raft = NULL;
cli_ctx_t ctxs[5];

typedef struct
{
    int type;
    int stat;
    union
    {
        msg_requestvote_t rv;
        msg_requestvote_response_t rvr;
        msg_appendentries_t ae;
        msg_appendentries_response_t aer;
    };
    int padding[100];
} msg_t;

msg_t msgs[5];

enum msg_stat {
    NONE = 0,
    AE,
    AER,
    RV,
    RVR
};

int read_pac(int node_id, const char* addr, int len)
{
    msgpack_unpacked result;
    if(msgpack_unpacker_buffer_capacity(ctxs[node_id].unp) < len) {
        msgpack_unpacker_reserve_buffer(ctxs[node_id].unp, len);
    }

    memcpy(msgpack_unpacker_buffer(ctxs[node_id].unp), addr, len);
    msgpack_unpacker_buffer_consumed(ctxs[node_id].unp, len);

    while(1) {
        uint64_t d;
        int ret = msgpack_unpacker_next(ctxs[node_id].unp, &result);
        if(MSGPACK_UNPACK_SUCCESS == ret) {
            msgpack_object obj = result.data;
            switch(msgs[node_id].stat) {
                case NONE:
                    d = msgs[node_id].type = obj.via.u64;
                    if(d == MSG_REQUESTVOTE_RESPONSE) {
                        msgs[node_id].stat = RVR;
                    } 
                    if(d == MSG_REQUESTVOTE) {
                        msgs[node_id].stat = RV;
                    } 
                    if(d == MSG_APPENDENTRIES) {
                        msgs[node_id].stat = AE;
                    } 
                    if(d == MSG_APPENDENTRIES_RESPONSE) {
                        msgs[node_id].stat = AER;
                    } 
                    break;

            }
        } else {
            break;
        }
    }

    msgpack_unpacked_destroy(&result);

    return len;
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
        .applylog                    = NULL,
        .persist_vote                = NULL,
        .persist_term                = NULL,
        .log_offer                   = NULL,
        .log_poll                    = NULL,
        .log_pop                     = NULL,
        .log                         = NULL
    };

    char* user_data = "test";

    raft_set_callbacks(raft, &raft_callbacks, user_data);


    /* TCP */

    serv_init(9000 + id, read_pac);

    sleep(1);

    for(int i = 0; i < 4; ++i)
        if(i != id) {
            strncpy(ctxs[i].host, "127.0.0.1", IP_STR_LEN);
            ctxs[i].node_id = i;
            ctxs[i].port = 9000+i;
            ctxs[i].buff = buffer_init();
            ctxs[i].r_cb = read_pac;
            ctxs[i].unp = msgpack_unpacker_new(128);
            cli_init(&ctxs[i]);
        }

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
