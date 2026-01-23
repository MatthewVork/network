#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cJSON.h"

#define BOARD_SIZE 10
int board[BOARD_SIZE][BOARD_SIZE] = {0};

void broadcast(int *clients, const char *msg) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "msg", msg);
    cJSON *j_board = cJSON_CreateArray();
    for (int i = 0; i < BOARD_SIZE; i++) {
        cJSON *row = cJSON_CreateArray();
        for (int j = 0; j < BOARD_SIZE; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(board[i][j]));
        }
        cJSON_AddItemToArray(j_board, row);
    }
    cJSON_AddItemToObject(root, "board", j_board);
    char *out = cJSON_PrintUnformatted(root);
    
    for (int i = 0; i < 2; i++) {
        send(clients[i], out, strlen(out), 0);
    }
    free(out);
    cJSON_Delete(root);
}

int main() {
    int server_fd, clients[2];
    struct sockaddr_in addr;
    int opt = 1, addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8888);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 2);

    printf("等待两名玩家...\n");
    for(int i=0; i<2; i++) {
        clients[i] = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
        printf("玩家 %d (FD:%d) 已连接\n", i+1, clients[i]);
    }

    broadcast(clients, "游戏开始！黑方先行");

    int turn = 0;
    char buf[1024];
    while (1) {
        memset(buf, 0, 1024);
        int len = recv(clients[turn], buf, 1024, 0);
        if (len <= 0) break;

        cJSON *json = cJSON_Parse(buf);
        if (json) {
            int x = cJSON_GetObjectItem(json, "x")->valueint;
            int y = cJSON_GetObjectItem(json, "y")->valueint;
            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && board[x][y] == 0) {
                board[x][y] = turn + 1;
                turn = 1 - turn;
                broadcast(clients, turn == 0 ? "轮到黑方" : "轮到白方");
            }
            cJSON_Delete(json);
        }
    }
    return 0;
}