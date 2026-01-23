#include "client.h"

// --- 实际定义变量 (分配内存) ---
int client_fd = -1;
volatile PlayerGlobalData g_player = {0}; 

void init_network()
{
    struct sockaddr_in serv_addr;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);
    // 使用你定义的地址
    inet_pton(AF_INET, "172.24.139.145", &serv_addr.sin_addr);

    printf("正在尝试接入服务器...\n");

    if(connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("network connect err"); 
        return;
    }
    else printf("网络连接成功！\n");
}