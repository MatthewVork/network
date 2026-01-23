#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void *loop(void *arg)
{
    int sockfd = (int)arg;
    while(1)
    {
        sleep(50);
        write(sockfd, "ping\r\n", 6);
    }
}

int main(void)
{
    //1.创建套接字socket， 
    int sockfd = socket(AF_INET, SOCK_STREAM,0);
    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }
    //2.连接服务器connect, 
    struct sockaddr_in saddr; //服务器地址
    memset(&saddr,0, sizeof(saddr));//清空
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8344);
    saddr.sin_addr.s_addr = inet_addr("119.91.109.180");
    int ret = connect(sockfd, (struct sockaddr*)&saddr,sizeof(saddr));
    if(ret < 0)
    {
        perror("connect");
        return -1;
    }
    //创建线程发送心跳包
    pthread_t id = 0;
    pthread_create(&id, NULL, loop, (void*)sockfd);
    pthread_detach(id);

    //发送数据
    //打包订阅命令
    const char *req = "cmd=1&uid=043e37350963486787b5c9fffa6a9b22&topic=KBSh1qiyv001,5Hxdwkmwq002&msg=no\r\n";
    write(sockfd, req, strlen(req));

    char buffer[128];
    while(1)
    {
        memset(buffer, 0, 128);
        int ret = read(sockfd, buffer, 128);
        if(ret <=0 )
        {
            break;
        }
        printf("%s\n", buffer);
    }
    
    //4.关闭套接字close
    close(sockfd);

}