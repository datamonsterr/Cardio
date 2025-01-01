#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "mpack.h"
#include "protocol.h"
#include "server.h"
#include "game.h"
#include "db.h"

#define dbconninfo "dbname=cardio user=postgres password=1234 host=localhost port=5433"

int compare_raw_bytes(char *b1, char *b2, int len);