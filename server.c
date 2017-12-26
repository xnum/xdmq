#include "server.h"

uv_tcp_t server;
read_cb cb = NULL;

void on_serv_read(uv_stream_t *cli, ssize_t st, const uv_buf_t *buf)
{
    if(st < 0) {
        slogf(ERR, "%s\n", uv_strerror(st));
        exit(1);
    }

    buffer_produced(cli->data, st);
    int consume = cb(buffer_begin(cli->data), buffer_size(cli->data));
    buffer_consume(cli->data, consume);
}

void on_serv_conn(uv_stream_t *handle, int rc)
{
    uv_tcp_t *cli = calloc(1, sizeof(uv_tcp_t));

    uv_tcp_init(uv_default_loop(), cli);

    uv_accept(handle, cli);

    cli->data = buffer_init();

    uv_read_start(cli, buffer_alloc, on_serv_read);
}

int serv_init(int port, read_cb pcb)
{
    const char *host = "0.0.0.0";
    struct sockaddr_in bind_addr;

    cb = pcb;

    uv_tcp_init(uv_default_loop(), &server);

    int rc = uv_ip4_addr(host, port, &bind_addr);
    if( rc < 0 ) {
        slogf(ERR, "uv_ipv4_addr(%s, %d, sockaddr_in) = %s\n", host, port, uv_strerror(rc));
        return 1;
    }
    slogf(INFO, "uv_ipv4_addr(%s, %d, sockaddr_in) = OK\n", host, port);

    rc = uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0);
    if( rc < 0 ) {
        slogf(ERR, "uv_tcp_bind(sockaddr, (%s, %d)) = %s\n", host, port, uv_strerror(rc));
        return 1;
    }
    slogf(INFO, "uv_tcp_bind(sockaddr, (%s, %d)) = OK\n", host, port);

    rc = uv_listen((uv_stream_t*) &server, 10/* backlog */, on_serv_conn);
    if( rc < 0 ) {
        slogf(ERR, "uv_listen() = %s\n", uv_strerror(rc));
        return 1;
    }
    slogf(INFO, "uv_listen() = OK\n");
}
