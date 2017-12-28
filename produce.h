#pragma once

#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "logger.h"
#include "buffer.h"

int producer_init(int port, read_cb pcb);
