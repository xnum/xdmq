#include "server.h"
#include <limits.h>

uv_tcp_t server;
read_cb cb = NULL;

static void handle_close(uv_handle_t *cli)
{
    free(cli->data);
    free(cli);
}

static void on_serv_read(uv_stream_t *cli, ssize_t st, const uv_buf_t *buf)
{
    if(st < 0) {
        slogf(ERR, "%s\n", se(st));
        uv_close((uv_handle_t*) cli, handle_close);
        return;
    }

    buffer_t t = cli->data;
    buffer_produced(t, st);

    while(buffer_size(t)) {
        if(t->data != INT_MAX) {
            //slogf(DEBUG, "consume packet %d\n", buffer_size(t));
            int consume = cb((int64_t)t->data, buffer_begin(t), buffer_size(t));
            if(consume == 0) break;
            buffer_consume(t, consume);
        } else if(buffer_size(t) >= sizeof(msg_handshake_t)) {
            msg_handshake_t *hs = (msg_handshake_t*) buffer_begin(t);
            if(hs->passcode != PASSCODE) {
                uv_close(cli, handle_close);
                return;
            }
            t->data = (void*) hs->node_id;
            //slogf(DEBUG, "handshake %d\n", hs->node_id);
            buffer_consume(t, sizeof(msg_handshake_t));
        } else {
            break;
        }
    }
}

static void on_serv_conn(uv_stream_t *handle, int rc)
{
    if(rc < 0) {
        //slogf(ERR, "%s\n", se(rc));
        return;
    }

    uv_tcp_t *cli = calloc(1, sizeof(uv_tcp_t));

    uv_tcp_init(uv_default_loop(), cli);

    uv_accept(handle, (uv_stream_t*) cli);

    cli->data = buffer_init();
    ((buffer_t)cli->data)->data = (void*) INT_MAX;

    slogf(DEBUG, "Connected\n");

    uv_read_start((uv_stream_t*) cli, buffer_alloc, on_serv_read);
}

int serv_init(int port, read_cb pcb)
{
    const char *host = "0.0.0.0";
    struct sockaddr_in bind_addr;

    cb = pcb;

    uv_tcp_init(uv_default_loop(), &server);

    int rc = uv_ip4_addr(host, port, &bind_addr);
    if( rc < 0 ) {
        slogf(ERR, "uv_ipv4_addr(%s, %d, sockaddr_in) = %s\n", host, port, se(rc));
        return 1;
    }
    slogf(INFO, "uv_ipv4_addr(%s, %d, sockaddr_in) = OK\n", host, port);

    rc = uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0);
    if( rc < 0 ) {
        slogf(ERR, "uv_tcp_bind(sockaddr, (%s, %d)) = %s\n", host, port, se(rc));
        return 1;
    }
    slogf(INFO, "uv_tcp_bind(sockaddr, (%s, %d)) = OK\n", host, port);

    rc = uv_listen((uv_stream_t*) &server, 10/* backlog */, on_serv_conn);
    if( rc < 0 ) {
        slogf(ERR, "uv_listen() = %s\n", se(rc));
        return 1;
    }
    slogf(INFO, "uv_listen() = OK\n");
}
