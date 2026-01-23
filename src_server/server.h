#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "./cJSON.h"

#define MAX_CLIENTS 50
#define MAX_ROOMS 10

typedef struct {
    int fd;
    char username[32];
    int is_authenticated;
    int room_id;
    int side;
} Player;

typedef struct {
    int room_id;
    int red_fd;
    int black_fd;
    int is_full;
    int host_fd;
    int red_ready;
    int black_ready;
    int turn;
    char board[90];
    time_t game_start_time;
} ChessRoom;

extern int client_fds[MAX_CLIENTS];
extern Player players[MAX_CLIENTS];
extern ChessRoom rooms[MAX_ROOMS];

// 核心修复：函数原型现在包含 type
void send_json_response(int fd, int type, const char* status, const char* msg);
void handle_register(int fd, cJSON *root);
void handle_login(int fd, cJSON *root, Player *p);

#endif