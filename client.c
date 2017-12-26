#include "client.h"

void on_cli_read(uv_stream_t *cli, ssize_t st, const uv_buf_t *buf)
{
    if(st < 0) {
        slogf(ERR, "%s\n", uv_strerror(st));
        exit(1);
    }

    cli_ctx_t *ctx = container_of((uv_tcp_t*)cli, cli_ctx_t, client);
    buffer_produced(ctx->buff, st);
    int consume = ctx->r_cb(buffer_begin(ctx->buff), buffer_size(ctx->buff));
    buffer_consume(ctx->buff, consume);
}

void on_cli_conn(uv_connect_t *conn, int st)
{
    if(st < 0) {
        slogf(ERR, "%s\n", uv_strerror(st));
        exit(1);
    }

    cli_ctx_t *ctx = container_of(conn, cli_ctx_t, conn);

    int rc = uv_read_start((uv_stream_t*)&ctx->client, buffer_alloc, on_cli_read);
}

int cli_init(cli_ctx_t* ctx)
{
    struct sockaddr_in bind_addr;

    int rc = uv_ip4_addr(ctx->host, ctx->port, &bind_addr);

    uv_tcp_init(uv_default_loop(), &ctx->client);

    ctx->client.data = &ctx->buff;

    uv_tcp_connect(&ctx->conn, &ctx->client, (struct sockaddr*) &bind_addr, on_cli_conn);
}
