#pragma once

#include <inttypes.h>

#define MSG_SIZE 32

typedef struct msg_exch_s {
    uint64_t id;
    char text[MSG_SIZE];
} msg_exch_t;
