#pragma once

#include <uv.h>
#include "logger.h"
#include "buffer.h"
#include "raft_ext.h"

int serv_init(int, read_cb);
