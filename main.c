#include <assert.h>
#include <msgpack.h>
#include <signal.h>
#include "raft.h"
#include "server.h"
#include "client.h"
#include "produce.h"
#include "raft_callbacks.h"
#include "persist.h"
#include "consume.h"
#include "msg.h"

#define PERIOD 1000

uv_timer_t timer;
raft_server_t *raft = NULL;
cli_ctx_t ctxs[5];
msg_t msgs[5];
QUEUE producers;

void entry_response_check()
{
    /*
    static int cmt_idx = 0;
    if(cmt_idx == raft_get_commit_idx(raft)) return;
    cmt_idx = raft_get_commit_idx(raft);
    */
    QUEUE *iter;
    QUEUE_FOREACH(iter, &producers) {
        produce_t *producer = QUEUE_DATA(iter, produce_t, wait_queue);
        QUEUE *it;
        QUEUE_FOREACH(it, &producer->msg_head) {
            p_entry_t *entry = QUEUE_DATA(it, p_entry_t, msg_queue);
            msg_entry_response_t *resp = &entry->resp;

            slogf(DEBUG, "Checking %llu\n", entry->req.id);
            int e = raft_msg_entry_response_committed(raft, resp);
            switch(e) {
                case 0: /* not yet */
                    break;
                case 1: /* done */
                default: /* error */
                    produce_response(producer, entry, e);
                    break;
            }
        }
    }
}

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

    int e = uv_try_write((uv_stream_t*) &ctx->client, &buf, 1);
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

    int e = uv_try_write((uv_stream_t*) &ctx->client, &buf, 1);
    if(e < 0) slogf(ERR, "%s\n", uv_strerror(e));

    msgpack_sbuffer_destroy(&sbuf);
}

int on_recv_msg(uv_tcp_t *conn, const char* addr, int len)
{
    produce_t *prod = container_of(conn, produce_t, conn);

    // leader check
    int leader_id = raft_get_current_leader(raft);

    msg_exch_t *msg = addr;
    // object init
    p_entry_t *entry = calloc(1, sizeof(p_entry_t));
    entry->req.data.buf = strndup(msg->text, member_size(msg_exch_t, text));
    entry->req.data.len = member_size(msg_exch_t, text);
    entry->req.id = msg->id;

    QUEUE_INSERT_TAIL(&prod->msg_head, &entry->msg_queue);

    int e = raft_recv_entry(raft, &entry->req, &entry->resp);
    switch (e) {
        case 0:
            break;
        case RAFT_ERR_NOT_LEADER:
            slogf(WARN, "I'm not leader, is %d\n", leader_id);
            produce_response(prod, entry, -8000 - leader_id);
            break;
        default:
            slogf(WARN, "raft_recv_entry = %d\n", e);
            break;
    }

    return 0;
}

int on_new_conn(produce_t *prod)
{
    QUEUE_INSERT_HEAD(&producers, &prod->wait_queue);

    return 0;
}

int on_disconn(produce_t *prod)
{
    QUEUE_REMOVE(&prod->wait_queue);

    return 0;
}

int read_pac(int64_t node_id, const char* addr, int len)
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
            //slogf(DEBUG, "Complete packet\n");
            /*
            slogf(DEBUG, "Role:%d curr_term:%d curr_idx:%d\n", raft_get_state(raft),
                    raft_get_current_term(raft),
                    raft_get_current_idx(raft)
                    );
            */
            if(msgs[node_id].type == MSG_REQUESTVOTE) {
                /*
                slogf(DEBUG, "Request Vote term:%d candidate_id:%d last_log_idx:%d last_log_term:%d\n",
                        msgs[node_id].rv.term,
                        msgs[node_id].rv.candidate_id,
                        msgs[node_id].rv.last_log_idx,
                        msgs[node_id].rv.last_log_term
                     );
                     */

                msg_requestvote_response_t rvr;
                raft_recv_requestvote(raft, ctxs[node_id].node, &msgs[node_id].rv, &rvr);
                // send to node ?
                send_requestvote_response(&ctxs[node_id], &rvr);
            }

            if(msgs[node_id].type == MSG_REQUESTVOTE_RESPONSE) {
                /*
                slogf(DEBUG, "Request Vote Response term:%d grant:%d\n", 
                        msgs[node_id].rvr.term,
                        msgs[node_id].rvr.vote_granted);
                        */
                raft_recv_requestvote_response(raft, ctxs[node_id].node, &msgs[node_id].rvr);
                slogf(INFO, "Leader is %d\n", raft_get_current_leader(raft));
            }

            if(msgs[node_id].type == MSG_APPENDENTRIES) {
                //if(msgs[node_id].ae.n_entries)
                    //slogf(DEBUG, "Append Entry entries# = %d\n", msgs[node_id].ae.n_entries);
                msg_appendentries_response_t aer;
                int e = raft_recv_appendentries(raft, ctxs[node_id].node, &msgs[node_id].ae, &aer);

                send_appendentries_response(&ctxs[node_id], &aer);
            }

            if(msgs[node_id].type == MSG_APPENDENTRIES_RESPONSE) {
                //slogf(DEBUG, "Append Entry Response\n");
                raft_recv_appendentries_response(raft, ctxs[node_id].node, &msgs[node_id].aer);
                entry_response_check();
            }

            free(msgs[node_id].ae.entries);
            memset(&msgs[node_id], 0, sizeof(msgs[0]));
        }

        uint64_t d;
        int ret = msgpack_unpacker_next(ctxs[node_id].unp, &result);
        if(MSGPACK_UNPACK_SUCCESS == ret) {

            msgpack_object obj = result.data;
            msg_t *msg = &msgs[node_id];
            msg_entry_t *entry;

            switch(msg->stat) {
                case NONE:
                    d = msg->type = obj.via.u64;
                    if(d == MSG_REQUESTVOTE) msg->stat = RV;
                    if(d == MSG_REQUESTVOTE_RESPONSE) msg->stat = RVR; 
                    if(d == MSG_APPENDENTRIES) msg->stat = AE;
                    if(d == MSG_APPENDENTRIES_RESPONSE) msg->stat = AER;
                    break;
                case RV:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_requestvote_t));
                    memcpy(&msg->rv, obj.via.bin.ptr, obj.via.bin.size);
                    msg->stat = DONE;
                    break;
                case AE:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_appendentries_t));
                    memcpy(&msg->ae, obj.via.bin.ptr, obj.via.bin.size);
                    if(msg->ae.n_entries == 0)
                        msg->stat = DONE;
                    else {
                        msg->stat = AE_ENTRIES;
                        msg->ae.entries = calloc(msg->ae.n_entries, sizeof(msg_entry_t));
                        msg->reserved = 0;
                    }
                    break;
                case AE_ENTRIES:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_entry_t));
                    memcpy(&msg->ae.entries[msg->reserved], obj.via.bin.ptr, obj.via.bin.size);
                    msg->stat = AE_ENTRY_BODY;
                    break;
                case AE_ENTRY_BODY:
                    entry = &msg->ae.entries[msg->reserved];
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == entry->data.len);
                    entry->data.buf = calloc(1, entry->data.len);
                    memcpy(entry->data.buf, obj.via.bin.ptr, entry->data.len);
                    if(msg->reserved+1 == msg->ae.n_entries)
                        msg->stat = DONE;
                    else {
                        msg->reserved++;
                        msg->stat = AE_ENTRIES;
                    }
                    break;
                case RVR:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_requestvote_response_t));
                    memcpy(&msg->rvr, obj.via.bin.ptr, obj.via.bin.size);
                    msg->stat = DONE;
                    break;
                case AER:
                    assert(obj.type == MSGPACK_OBJECT_BIN);
                    assert(obj.via.bin.size == sizeof(msg_appendentries_response_t));
                    memcpy(&msg->aer, obj.via.bin.ptr, obj.via.bin.size);
                    msg->stat = DONE;
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

    raft_periodic(raft, PERIOD);

    entry_response_check();
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
    srand(time(NULL));

    QUEUE_INIT(&producers);

    signal(SIGPIPE, SIG_IGN);

    enable_coredump();

    assert(argc == 3);
    int64_t id = atoi(argv[1]);
    int total = atoi(argv[2]);
    slogf(INFO, "id = %lld\n", id);

    persist_init("xdmq.mmap", id);

    uv_timer_init(uv_default_loop(), &timer);
    uv_timer_start(&timer, on_time, 0, PERIOD);

    raft = raft_new();
    for(int i = 0; i < total; ++i)
        ctxs[i].node = raft_add_node(raft, &ctxs[i], i, id == i);

    persist_load(raft);

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

    void* user_data = id;

    raft_set_callbacks(raft, &raft_callbacks, user_data);

    /* TCP */

    serv_init(9000 + id, read_pac);

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

    /* Producer */
    produce_init(8000+id);
    produce_set_read_callback(on_recv_msg);
    produce_set_connected_callback(on_new_conn);
    produce_set_disconnected_callback(on_disconn);
    consume_init(7000+id);

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
