#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // 提供网络地址转换和 Socket 定义
#include <sys/select.h> // 提供 select 多路复用函数
#include "cJSON.h"

#define PORT 8888       // 服务器监听端口
#define MAX_CLIENTS 50  // 最大同时在线人数

// 记录所有连接上来的客户端的文件描述符 (FD)
int client_fds[MAX_CLIENTS]; 

int main() {
    int listen_fd; // 服务器监听用的 Socket
    struct sockaddr_in serv_addr; // 服务器的网络信息结构体

    // 1. 创建 Socket (AF_INET: IPv4; SOCK_STREAM: TCP协议)
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 【重要】设置端口复用：防止服务器崩溃重启时提示“Address already in use”
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //定义了一个开关变量opt， 1 -> 开启， 0 -> 关闭
    //第二个函数是设置套接字选项，SOL_SOCKET -> 通用， SO_REUSEADDR -> 复用地址 
    //因为TCP协议为了保证数据的完整传输，底层在释放端口之前会进入一段等待时间，若重启可能导致时间等待
    //opt：代表开启这个功能，通过这个地址找这个值，然后读取的内存大小由Sizeof决定 

    // 配置服务器信息
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网卡 IP
    serv_addr.sin_port = htons(PORT);             // 转换为网络字节序

    // 2. 绑定 (Bind): 把 Socket 和端口号 8888 绑定在一起
    if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed"); 
        exit(EXIT_FAILURE);
    }

    // 3. 监听 (Listen): 开始等待客户端连接，最大排队队列为 5
    if(listen(listen_fd, 5) < 0)
    {
        perror("Listen failed"); 
        exit(EXIT_FAILURE);
    }

    printf("象棋服务器已启动，正在监听端口: %d...\n", PORT);

    // 初始化客户端数组，全部设为 -1 (代表空位)
    for (int i = 0; i < MAX_CLIENTS; i++) client_fds[i] = -1;

    // --- 服务器核心主循环 ---
    fd_set read_fds; // 定义一个“监听集合”，select 会监控集合里的所有 FD
    while (1) {
        FD_ZERO(&read_fds);       // 每次循环前清空集合
        FD_SET(listen_fd, &read_fds); // 把监听 Socket 加入集合，看有没有新人在敲门
        int max_fd = listen_fd;

        // 将所有已经连接上的玩家 FD 也加入监听集合，看他们有没有发消息（走棋、聊天）
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) max_fd = client_fds[i]; // 记录最大的 FD 供 select 使用
            }
        }

        // 4. select 多路复用：这是最关键的一步
        // 它会在这里阻塞，直到有任何一个 FD 有动态（有人连接 或 有人发数据）才会返回
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) continue;

        // 【事件 A】：新玩家连接请求
        if (FD_ISSET(listen_fd, &read_fds)) {
            int new_fd = accept(listen_fd, NULL, NULL); // 接受连接
            // 在数组里找一个空位 (-1) 存入新玩家的 FD
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = new_fd;
                    printf("【系统】新玩家已连接: FD %d\n", new_fd);
                    break;
                }
            }
        }

        // 【事件 B】：已连接的玩家发来了数据
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                char buf[2048] = {0}; // 接收缓冲区
                int len = recv(fd, buf, sizeof(buf), 0); // 尝试接收数据

                if (len <= 0) { 
                    // 如果接收长度 <= 0，代表玩家断开了连接
                    printf("【系统】玩家退出: FD %d\n", fd);
                    close(fd);         // 关闭这个 Socket
                    client_fds[i] = -1; // 数组位置腾出来给别人
                } else {
                    // 接收到了真正的数据，准备进行业务逻辑处理
                    printf("【收到数据】来自 FD %d: %s\n", fd, buf);
                    
                    /* 这里就是之后的逻辑入口：
                       1. cJSON_Parse(buf) 解析出命令类型。
                       2. 如果是 REGISTER，就调注册函数。
                       3. 如果是 MOVE，就转发给对手。
                    */
                }
            }
        }
    }
    return 0;
}