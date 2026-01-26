#include "client.h"

// --- 实际定义变量 (分配内存) ---
int client_fd = -1;
volatile PlayerGlobalData g_player = {0}; 

void init_network() {
    struct sockaddr_in serv_addr;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 增加：允许端口重用 (防止客户端频繁重启导致端口被占)
    int opt = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);
    
    // 【核心修改】先尝试本地回路
    if (inet_pton(AF_INET, "172.28.17.136", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return;
    }

    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("连接服务器失败 (Connect Failed)");
        client_fd = -1;
    } else {
        printf("恭喜！网络连接成功！FD=%d\n", client_fd);
    }
}