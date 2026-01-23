#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>  // 增加线程支持
#include "cJSON.h"

// 声明全局变量，让其他文件能看见
extern int client_fd;

void init_network();

typedef struct {
    volatile int has_update; // 信号量：1 表示有新包
    int msg_type;           // 1-注册, 2-登录, 3-落子
    char status[16];        // "success" 或 "error"
    char msg_text[128];     // 服务器返回的消息文字
    
    char username[32];
    int room_id;
    char role[16];          
    int is_my_turn;         
} PlayerGlobalData;

extern volatile PlayerGlobalData g_player; // 声明全局玩家数据

#endif