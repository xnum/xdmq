#include <assert.h>
#include <msgpack.h>
#include <signal.h>
#include "raft.h"
#include "server.h"
#include "client.h"
#include "raft_callbacks.h"

uv_timer_t timer;
raft_server_t *raft = NULL;
cli_ctx_t ctxs[5];
msg_t msgs[5];

void send_requestvote_response(cli_ctx_t *ctx, msg_requestvote_response_t *r)
{
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_int(&pk, MSG_REQUESTVOTE_RESPONSE);
    msgpack_pack_bin(&pk, sizeof(msg_requestvote_response_t));
    msgpack_pack_bin_body(&pk, r, sizeof(msg_requestvote_response_t));

    uv_buf_t buf = uv_buf_init(sbuf.data, sbuf.size);

    int e = uv_try_write(&ctx->client, &buf, 1);
    if(e < 0) slogf(ERR, "%s\n", uv_strerror(e));

    msgpack_sbuffer_destroy(&sbuf);
}

void send_appendentries_response(cli_ctx_t *ctx, msg_appendentries_response_t *r)
{
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_int(&pk, MSG_APPENDENTRIES_RESPONSE);
    msgpack_pack_bin(&pk, sizeof(msg_appendentries_response_t));
    msgpack_pack_bin_body(&pk, r, sizeof(msg_appendentries_response_t));

    uv_buf_t buf = uv_buf_init(sbuf.data, sbuf.size);

    int e = uv_try_write(&ctx->client, &buf, 1);
    if(e < 0) slogf(ERR, "%s\n", uv_strerror(e));

    msgpack_sbuffer_destroy(&sbuf);
}

int read_pac(int node_id, const char* addr, int len)
{
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);
    if(msgpack_unpacker_buffer_capacity(ctxs[node_id].unp) < len) {
        msgpack_unpacker_reserve_buffer(ctxs[node_id].unp, len);
    }

    memcpy(msgpack_unpacker_buffer(ctxs[node_id].unp), addr, len);
    msgpack_unpacker_buffer_consumed(ctxs[node_id].unp, len);

    while(1) {
        if(msgs[node_id].stat == DONE) {
            slogf(DEBUG, "Complete packet\n");
            slogf(DEBUG, "Who am I? %d\n", raft_get_state(raft));
            if(msgs[node_id].type == MSG_REQUESTVOTE) {
                slogf(DEBUG, "Request Vote term:%d candidate_id:%d last_log_idx:%d last_log_term:%d\n",
                        msgs[node_id].rv.term,
                        msgs[node_id].rv.candidate_id,
                        msgs[node_id].rv.last_log_idx,
                        msgs[node_id].rv.last_log_term
                        );

                msg_requestvote_response_t rvr;
                raft_recv_requestvote(raft, ctxs[node_id].node, &msgs[node_id].rv, &rvr);
                // send to node ?
                send_requestvote_response(&ctxs[node_id], &rvr);
            }

            if(msgs[node_id].type == MSG_REQUESTVOTE_RESPONSE) {
                slogf(DEBUG, "Request Vote Response term:%d grant:%d\n", 
                        msgs[node_id].rvr.term,
                        msgs[node_id].rvr.vote_granted);
                raft_recv_requestvote_response(raft, ctxs[node_id].node, &msgs[node_id].rvr);
            }

            if(msgs[node_id].type == MSG_APPENDENTRIES) {
                slogf(DEBUG, "Append Entry\n");
                msg_appendentries_response_t aer;
                raft_recv_appendentries(raft, ctxs[node_id].node, &msgs[node_id].ae, &aer);
                
                send_appendentries_response(&ctxs[node_id], &aer);
            }

            if(msgs[node_id].type == MSG_APPENDENTRIES_RESPONSE) {
                slogf(DEBUG, "Append Entry Response\n");
                raft_recv_appendentries_response(raft, ctxs[node_id].node, &msgs[node_id].aer);
            }

            memset(&msgs[node_id], 0, sizeof(msgs[0]));
        }

        uint64_t d;
        int ret = msgpack_unpacker_next(ctxs[node_id].unp, &result);
        if(MSGPACK_UNPACK_SUCCESS == ret) {

            msgpack_object obj = result.data;

            switch(msgs[node_id].stat) {
                case NONE:
                    d = msgs[node_id].type = obj.via.u64;
                    if(d == MSG_REQUESTVOTE) msgs[node_id].stat = RV;
                    if(d == MSG_REQUESTVOTE_RESPONSE) msgs[node_id].stat = RVR; 
                    if(d == MSG_APPENDENTRIES) msgs[node_id].stat = AE;
                    if(d == MSG_APPENDENTRIES_RESPONSE) msgs[node_id].stat = AER;
                    break;
                case RV:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_requestvote_t));
                    memcpy(&msgs[node_id].rv, obj.via.bin.ptr, obj.via.bin.size);
                    msgs[node_id].stat = DONE;
                    break;
                case AE:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_appendentries_t));
                    memcpy(&msgs[node_id].ae, obj.via.bin.ptr, obj.via.bin.size);
                    // TODO log?
                    msgs[node_id].stat = DONE;
                    break;
                case RVR:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_requestvote_response_t));
                    memcpy(&msgs[node_id].rvr, obj.via.bin.ptr, obj.via.bin.size);
                    msgs[node_id].stat = DONE;
                    break;
                case AER:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_appendentries_response_t));
                    memcpy(&msgs[node_id].aer, obj.via.bin.ptr, obj.via.bin.size);
                    msgs[node_id].stat = DONE;
                    break;
                default:
                    break; 
            }
        } else {
            break;
        }
    }

    msgpack_unpacked_destroy(&result);

    return len;
}

void on_time(uv_timer_t *t)
{
    for(int i = 0; i < 4; ++i) {
        if(ctxs[i].status == 0 && ctxs[i].port != 0)
            cli_init(&ctxs[i]);
    }

    raft_periodic(raft, 200);
}

void log(
     raft_server_t* raft,
     raft_node_t* node,
     void *user_data,
     const char *buf
 )
{
    slogf(DEBUG, "%s\n", buf);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    assert(argc == 3);
    int id = atoi(argv[1]);
    int total = atoi(argv[2]);
    slogf(INFO, "id = %d\n", id);

    uv_timer_init(uv_default_loop(), &timer);
    uv_timer_start(&timer, on_time, 200, 200);

    raft = raft_new();
    for(int i = 0; i < total; ++i)
        ctxs[i].node = raft_add_node(raft, &ctxs[i], i, id == i);

    raft_cbs_t raft_callbacks = {
        .send_requestvote            = send_requestvote,
        .send_appendentries          = send_appendentries,
        .applylog                    = applylog,
        .persist_vote                = persist_vote,
        .persist_term                = persist_term,
        .log_offer                   = logentry_offer,
        .log_poll                    = logentry_poll,
        .log_pop                     = logentry_pop,
        .log                         = NULL
    };

    char* user_data = "test";

    raft_set_callbacks(raft, &raft_callbacks, user_data);


    /* TCP */

    serv_init(9000 + id, read_pac);

    sleep(1);

    for(int i = 0; i < total; ++i)
        if(i != id) {
            strncpy(ctxs[i].host, "127.0.0.1", IP_STR_LEN);
            ctxs[i].node_id = i;
            ctxs[i].port = 9000+i;
            ctxs[i].buff = buffer_init();
            ctxs[i].r_cb = read_pac;
            ctxs[i].unp = msgpack_unpacker_new(128);
            ctxs[i].status = 0;
            ctxs[i].self_id = id;
        }

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
