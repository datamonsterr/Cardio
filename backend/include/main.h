#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "db.h"
#include "game.h"
#include "logger.h"
#include "mpack.h"
#include "protocol.h"
#include "server.h"

#define dbconninfo "dbname=cardio user=root password=1234 host=localhost port=5433"
#define MAIN_LOG "server.log"

int compare_raw_bytes(char* b1, char* b2, int len);