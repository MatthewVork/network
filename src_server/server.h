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

// --- 玩家结构体 ---
typedef struct {
    int fd;                 // 玩家的 Socket 编号
    char username[32];      // 玩家姓名
    int is_authenticated;   // 登录状态：0-未登录，1-已登录
    int room_id;            // 所在房间号：-1 表示在大厅
    int side;               // 阵营：0-红方，1-黑方
} Player;

// --- 房间结构体 ---
typedef struct {
    int room_id;
    int red_fd;
    int black_fd;
    int is_full;         // -1:空闲, 0:等待中, 1:对局中
    int host_fd;         // 房主 FD
    int red_ready;       // 红方准备状态
    int black_ready;     // 黑方准备状态
    int turn;            // 0:红方走, 1:黑方走
    char board[90];      // 记录棋盘布局
    time_t game_start_time;
} ChessRoom;

// 函数声明
void handle_register(int fd, cJSON *root);
void handle_login(int fd, cJSON *root, Player *p);
void handle_get_rooms(int fd);
void handle_join_room(int fd, cJSON *root, Player *p);
void handle_ready(int fd, Player *p);
void handle_move(int fd, cJSON *root, Player *p);
void handle_disconnect(int fd);
void send_json_response(int fd, const char* status, const char* msg);

#endif