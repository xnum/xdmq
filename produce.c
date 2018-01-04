#include "produce.h"
#include "msg.h"

static uv_tcp_t mq_serv;

static prod_read_cb r_cb = NULL;
static prod_conn_cb c_cb = NULL;
static prod_disconn_cb d_cb = NULL;

typedef struct {
    char *msg;
    p_entry_t *ety;
} prod_data_t;

void handle_write(uv_write_t *req, int rc)
{
    prod_data_t *data = req->data;
    char* msg = data->msg;
    p_entry_t* ety = data->ety;

    free(req);
    free(data);
    free(ety);
    free(msg);
}

void produce_response(produce_t * prod, p_entry_t * ety, int rc)
{
    QUEUE_REMOVE(&ety->msg_queue);
    prod_data_t *data = calloc(1, sizeof(prod_data_t));
    char *msg = calloc(1, 128);
    data->msg = msg;
    data->ety = ety;
    uv_write_t *req = calloc(1, sizeof(uv_write_t));
    req->data = data;

    snprintf(msg, 128, "%s[%x]", rc==1?"OK":"ERR", ety->req.id);

    uv_buf_t buf_wrap = uv_buf_init(msg, 128);
    int e = uv_write(req, (uv_stream_t*) &prod->conn, &buf_wrap, 1, handle_write);
}

static void handle_close(uv_handle_t *cli)
{
    produce_t *prod = container_of((uv_tcp_t*)cli, produce_t, conn);
    QUEUE_REMOVE(&prod->wait_queue);
    while(!QUEUE_EMPTY(&prod->msg_head)) {
        QUEUE *it = QUEUE_HEAD(&prod->msg_head);
        p_entry_t *ety = QUEUE_DATA(it, p_entry_t, msg_queue);
        QUEUE_REMOVE(&ety->msg_queue);
        free(ety);
    }
    free(cli->data);
    free(prod);
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

    char *ptr = buffer_begin(t);
    for(int idx = 1; idx*sizeof(msg_exch_t) <= buffer_size(t); ++idx) {
        r_cb((uv_tcp_t*) cli, ptr+((idx-1) * sizeof(msg_exch_t)), sizeof(msg_exch_t));
    }
    buffer_consume(t, (buffer_size(t)/sizeof(msg_exch_t))*sizeof(msg_exch_t));
}

static void on_serv_conn(uv_stream_t *handle, int rc)
{
    if(rc < 0) {
        slogf(ERR, "%s\n", se(rc));
        return;
    }

    produce_t *prod = calloc(1, sizeof(produce_t));
    uv_tcp_t *cli = &prod->conn;
    QUEUE_INIT(&prod->wait_queue);
    QUEUE_INIT(&prod->msg_head);

    uv_tcp_init(uv_default_loop(), cli);

    uv_accept(handle, (uv_stream_t*) cli);

    cli->data = buffer_init();
    ((buffer_t)cli->data)->data = 0;

    slogf(DEBUG, "Connected\n");

    if(c_cb) c_cb(prod);

    uv_read_start((uv_stream_t*) cli, buffer_alloc, on_serv_read);
}

void produce_set_read_callback(prod_read_cb cb)
{
    r_cb = cb; 
}

void produce_set_connected_callback(prod_conn_cb cb)
{
    c_cb = cb;
}

void produce_set_disconnected_callback(prod_disconn_cb cb)
{
    d_cb = cb;
}

int produce_init(int port)
{
    const char *host = "0.0.0.0";
    struct sockaddr_in bind_addr;

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
