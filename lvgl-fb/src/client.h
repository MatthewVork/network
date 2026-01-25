#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "cJSON.h"

// --- 定义全局结构体 ---
typedef struct {
    volatile int has_update; // 信号量
    int msg_type;           // 消息类型
    char status[16];        // success/error
    char msg_text[128];     // 提示文字
    char username[32];
    int room_id;
    int side;               // 阵营：1红 2黑
    char role[16];          
    int is_my_turn;         
} PlayerGlobalData;

// --- 声明外部变量 ---
extern int client_fd; 
extern volatile PlayerGlobalData g_player; 

void init_network();

#endif