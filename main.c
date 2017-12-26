#include <assert.h>
#include <msgpack.h>
#include "raft.h"
#include "server.h"
#include "client.h"
#include "raft_callbacks.h"


raft_server_t *raft = NULL;
cli_ctx_t ctxs[5];


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
            ctxs[i].port = 9000+i;
            ctxs[i].buff = buffer_init();
            ctxs[i].r_cb = read_pac;
            cli_init(&ctxs[i]);
        }

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
