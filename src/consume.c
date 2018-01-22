#include "consume.h"
#include "persist.h"
#include "b64.h"


static uv_tcp_t serv;

static int do_cmd(uv_stream_t *cli, char* addr, int len)
{
    len--;
    printf("msg = %s %d\n", addr, len);
    int cmt_idx = get_cmt_idx();
    /* GETA N M: get entry from N to M */
    if( 0 == memcmp(addr, "GETA", 4)) {
        int n = -1, m = -1;
        sscanf(addr, "%*s %d %d", &n, &m);
        if(m>=cmt_idx) {
            char buf[128] = {};
            snprintf(buf, 128, "Out of index\n");
            uv_buf_t buf_wrap = uv_buf_init(buf, strlen(buf));
            uv_try_write(cli, &buf_wrap, 1);
        } else {
            if(m == 0) m = cmt_idx;
            for(int k = n; k < m; ++k) {
                entry_t *ety = get_entry(k);

                char buf[256] = {};
                snprintf(buf, 256, "%d %s\n", k, ety->text);
                uv_buf_t buf_wrap = uv_buf_init(buf, strlen(buf));
                uv_try_write(cli, &buf_wrap, 1);
            }
        }
    }
    /* INFO: print queue info */
    else if( 0 == memcmp(addr, "INFO", 4)) {
        char buf[128] = {};
        snprintf(buf, 128, "Commit index = %d\n", cmt_idx);
        uv_buf_t buf_wrap = uv_buf_init(buf, strlen(buf));
        uv_try_write(cli, &buf_wrap, 1);
    }
    else {
        return -1;
    }

    return len;
}

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
                int e = do_cmd(cli, ptr, i);
                if(e < 0) {
                    uv_close(cli, handle_close);
                    return;
                }
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

void consume_init(int port)
{
    const char *host = "0.0.0.0";
    struct sockaddr_in bind_addr;

    uv_tcp_init(uv_default_loop(), &serv);

    int rc = uv_ip4_addr(host, port, &bind_addr);
    if( rc < 0 ) {
        slogf(ERR, "uv_ipv4_addr(%s, %d, sockaddr_in) = %s\n", host, port, se(rc));
        return ;
    }
    slogf(INFO, "uv_ipv4_addr(%s, %d, sockaddr_in) = OK\n", host, port);

    rc = uv_tcp_bind(&serv, (const struct sockaddr*)&bind_addr, 0);
    if( rc < 0 ) {
        slogf(ERR, "uv_tcp_bind(sockaddr, (%s, %d)) = %s\n", host, port, se(rc));
        return ;
    }
    slogf(INFO, "uv_tcp_bind(sockaddr, (%s, %d)) = OK\n", host, port);

    rc = uv_listen((uv_stream_t*) &serv, 10/* backlog */, on_serv_conn);
    if( rc < 0 ) {
        slogf(ERR, "uv_listen() = %s\n", se(rc));
        return ;
    }
    slogf(INFO, "uv_listen() = OK\n");
}
