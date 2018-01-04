#include "client.h"

static void handle_close(uv_handle_t *cli)
{
    cli_ctx_t *ctx = container_of((uv_tcp_t*)cli, cli_ctx_t, client);
    ctx->status = 0;
}

static void on_cli_read(uv_stream_t *cli, ssize_t st, const uv_buf_t *buf)
{
    cli_ctx_t *ctx = container_of((uv_tcp_t*)cli, cli_ctx_t, client);

    if(st < 0) {
        slogf(ERR, "%s\n", se(st));
        uv_close(&ctx->client, handle_close);
        return;
    }

    buffer_produced(ctx->buff, st);
    int consume = ctx->r_cb(ctx->node_id, buffer_begin(ctx->buff), buffer_size(ctx->buff));
    buffer_consume(ctx->buff, consume);
}

static void on_cli_conn(uv_connect_t *conn, int st)
{
    cli_ctx_t *ctx = container_of(conn, cli_ctx_t, conn);

    if(st < 0) {
        //slogf(ERR, "%s:%d %s\n", ctx->host, ctx->port, se(st));
        ctx->status = 0;
        return;
        //exit(1);
    }

    int rc = uv_read_start((uv_stream_t*)&ctx->client, buffer_alloc, on_cli_read);

    msg_handshake_t hs;
    hs.node_id = ctx->self_id;

    uv_buf_t bufs = uv_buf_init(&hs, sizeof(hs));
    int e = uv_try_write(&ctx->client, &bufs, 1);
    if(e < 0) slogf(ERR, "Write %s\n", uv_strerror(e));
}

int cli_init(cli_ctx_t* ctx)
{
    ctx->status = 0;

    struct sockaddr_in bind_addr;

    int rc = uv_ip4_addr(ctx->host, ctx->port, &bind_addr);

    uv_tcp_init(uv_default_loop(), &ctx->client);

    ctx->client.data = ctx->buff;

    int e = uv_tcp_connect(&ctx->conn, &ctx->client, (struct sockaddr*) &bind_addr, on_cli_conn);
    ctx->status = 1;
    if(e < 0) ctx->status = 0;
}
