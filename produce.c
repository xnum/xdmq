#include "produce.h"

static uv_tcp_t mq_serv;

static read_cb cb = NULL;

static void handle_close(uv_tcp_t *cli)
{
    free(cli->data);
    free(cli);
}

static void on_serv_read(uv_stream_t *cli, ssize_t st, const uv_buf_t *buf)
{
    if(st < 0) {
        slogf(ERR, "%s\n", se(st));
        uv_close(cli, handle_close);
        return;
    }

    buffer_t t = cli->data;
    buffer_produced(t, st);

    bool found = false;
    do {
        found = false;
        char *ptr = buffer_begin(t);
        for(int i = 0; i < buffer_size(t); ++i) {
            if(ptr[i] == '\n') {
                int e = cb(-9999, ptr, i);
                char *msg = NULL;
                switch(e) {
                    case 0:
                        msg = "OK\n\0";
                        break;
                    case 1:
                        msg = "Fail\n\0";
                        break;
                    default:
                        msg = "Error!\n\0";
                        break;
                }
                uv_buf_t buf = uv_buf_init(msg, strlen(msg));
                uv_try_write(cli, &buf, 1);
                buffer_consume(t, i+1);
                found = true;
                break;
            } 
        }
    } while(found);
}

static void on_serv_conn(uv_stream_t *handle, int rc)
{
    if(rc < 0) {
        slogf(ERR, "%s\n", se(rc));
        return;
    }

    uv_tcp_t *cli = calloc(1, sizeof(uv_tcp_t));

    uv_tcp_init(uv_default_loop(), cli);

    uv_accept(handle, cli);

    cli->data = buffer_init();
    ((buffer_t)cli->data)->data = 0;

    slogf(DEBUG, "Connected\n");

    uv_read_start(cli, buffer_alloc, on_serv_read);
}

int producer_init(int port, read_cb pcb)
{
    const char *host = "0.0.0.0";
    struct sockaddr_in bind_addr;

    cb = pcb;

    uv_tcp_init(uv_default_loop(), &mq_serv);

    int rc = uv_ip4_addr(host, port, &bind_addr);
    if( rc < 0 ) {
        slogf(ERR, "uv_ipv4_addr(%s, %d, sockaddr_in) = %s\n", host, port, se(rc));
        return 1;
    }
    slogf(INFO, "uv_ipv4_addr(%s, %d, sockaddr_in) = OK\n", host, port);

    rc = uv_tcp_bind(&mq_serv, (const struct sockaddr*)&bind_addr, 0);
    if( rc < 0 ) {
        slogf(ERR, "uv_tcp_bind(sockaddr, (%s, %d)) = %s\n", host, port, se(rc));
        return 1;
    }
    slogf(INFO, "uv_tcp_bind(sockaddr, (%s, %d)) = OK\n", host, port);

    rc = uv_listen((uv_stream_t*) &mq_serv, 10/* backlog */, on_serv_conn);
    if( rc < 0 ) {
        slogf(ERR, "uv_listen() = %s\n", se(rc));
        return 1;
    }
    slogf(INFO, "uv_listen() = OK\n");
}
