#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "cJSON.h"

int sock;

// 负责渲染棋盘的函数
void draw_board(cJSON *j_board) {
    if (!j_board) return;
    system("clear"); // 开发板上如果不支持 clear，可以换成 printf("\033[H\033[J");
    printf("==== 联网五子棋 Demo ====\n");
    printf("  0 1 2 3 4 5 6 7 8 9\n");
    int size = cJSON_GetArraySize(j_board);
    for (int i = 0; i < size; i++) {
        printf("%d ", i);
        cJSON *row = cJSON_GetArrayItem(j_board, i);
        int row_size = cJSON_GetArraySize(row);
        for (int j = 0; j < row_size; j++) {
            int val = cJSON_GetArrayItem(row, j)->valueint;
            if(val == 0) printf(". ");
            else if(val == 1) printf("X "); // P1
            else printf("O ");              // P2
        }
        printf("\n");
    }
}

// 接收线程：实时监听服务器消息
void* recv_handler(void* arg) {
    char buf[8192];
    while (1) {
        memset(buf, 0, sizeof(buf));
        int len = recv(sock, buf, sizeof(buf), 0);
        if (len <= 0) {
            printf("\n与服务器断开连接...\n");
            exit(0);
        }

        cJSON *root = cJSON_Parse(buf);
        if (root) {
            cJSON *board = cJSON_GetObjectItem(root, "board");
            cJSON *msg = cJSON_GetObjectItem(root, "msg");
            
            draw_board(board);
            if (msg) printf("\n[系统消息]: %s\n", msg->valuestring);
            printf("请输入你的坐标 (行 列): ");
            fflush(stdout); // 强制刷新缓冲区，确保提示符显示
            cJSON_Delete(root);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);
    inet_pton(AF_INET, "192.168.172.26", &serv_addr.sin_addr); // 填你的 Windows IP

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("连接失败");
        return -1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, recv_handler, NULL);

    while (1) {
        int x, y;
        if (scanf("%d %d", &x, &y) == 2) {
            char move_json[64];
            sprintf(move_json, "{\"x\":%d, \"y\":%d}", x, y);
            send(sock, move_json, strlen(move_json), 0);
        }
    }
    return 0;
}