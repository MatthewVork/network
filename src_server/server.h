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
    int side; // 1:红, 2:黑
} Player;

typedef struct {
    int room_id;
    int is_active;    // 标记位：0空闲，1占用
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

// 全局变量声明
extern int client_fds[MAX_CLIENTS];
extern Player players[MAX_CLIENTS];
extern ChessRoom rooms[MAX_ROOMS];

// 函数声明
void send_json_response(int fd, int type, const char* status, const char* msg);
void handle_register(int fd, cJSON *root);
void handle_login(int fd, cJSON *root, Player *p);
void handle_create_room(int fd, cJSON *root, Player *p);
// 【修复：补全参数和声明】
void handle_join_room(int fd, cJSON *root, Player *p);
void handle_leave_room(int fd, cJSON *root, Player *p); 
void handle_ready(int fd, cJSON *root, Player *p);

#endif