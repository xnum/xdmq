#include "buffer.h"

buffer_t buffer_init()
{
    buffer_t res = (buffer_t) malloc(sizeof(struct buffer_s));
    res->begin = &res->buf;
    res->max_size = _BUFFER_SIZE;
    res->ava_size = 0;
    res->data = NULL;
    return res;
}

void buffer_clear(buffer_t t)
{
    t->ava_size = 0;
    t->max_size = _BUFFER_SIZE;
    t->data = NULL;
}

int buffer_max_size(buffer_t t)
{
    return t->max_size - t->ava_size;
}

// Read時使用，判斷目前擁有的資料size
uint32_t buffer_size(buffer_t t)
{
    assert(t != NULL);
    return t->ava_size;
}

// Read時使用，取得buf指標
char* buffer_begin(buffer_t t)
{
    assert(t != NULL);
    return t->begin;
}

// Read後使用，消耗資料
void buffer_consume(buffer_t t, int n)
{
    assert(t != NULL);
    assert(n!=0);
    assert(t->ava_size-n >= 0);
    //memmove(t->buf, t->buf+n, t->ava_size-n);
    t->ava_size -= n;
    t->begin += n;
}

// Write時使用，提供目前應寫入的指標位置
char* buffer_ava_loc(buffer_t t)
{
    assert(t != NULL);
    return t->begin + t->ava_size;
}

// Write時使用，提供目前還可以寫入的size，需傳入想要的size
int buffer_ava_size(buffer_t t, int desired_size)
{
    assert(t != NULL);
    int inner_size = t->max_size - t->ava_size;
    if(desired_size > inner_size)
        return inner_size;
    else
        return desired_size;
}

// Write後使用，增加目前的資料size
void buffer_produced(buffer_t t, int written_size)
{
    assert(t != NULL);
    t->ava_size += written_size;
}

void buffer_append(buffer_t t, const char *buf, int len)
{
    assert(t != NULL);
    assert(t->ava_size + len <= t->max_size);

    memcpy(buffer_ava_loc(t), buf, len);

    t->ava_size += len;
}

// Helper (read's alloc cb)
void buffer_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) 
{
    buffer_t t = handle->data;
    assert(t != NULL);

    if(t->ava_size)
        memmove(t->buf, t->begin, t->ava_size);
    t->begin = t->buf;
    buf->base = buffer_ava_loc(t);
    buf->len = buffer_ava_size(t, suggested_size);
}


