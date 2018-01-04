#pragma once

#include <uv.h>
#include "logger.h"
#include "buffer.h"
#include "raft_ext.h"
#include "produce.h"

typedef int(*read_cb)(int64_t, const char*, int);

int serv_init(int, read_cb);
