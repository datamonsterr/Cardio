#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "generic_map.h"
#include "mpack.h"
#include "protocol.h"
#include "server.h"

#define MAXLINE 65540

Map *decode_msgpack(const char *data, size_t len);
int string_compare(const void *a, const void *b);