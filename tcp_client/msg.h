#pragma once

#include <inttypes.h>

typedef struct msg_exch_s {
    uint64_t id;
    char text[32];   // 32 
} msg_exch_t;
