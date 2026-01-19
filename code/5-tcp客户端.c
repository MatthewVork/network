#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = connect(sockfd, (struct sockaddr*)&saddr,sizeof(saddr));
    if(ret < 0)
    {
        perror("connect");
        return -1;
    }
    //3.发送和接收数据write,send,read,recv, 
    //发送数据
    write(sockfd, "helloworld", 10);
    send(sockfd, "linux", 5, 0);

    //接收数据
    char buffer[128];
    int size = read(sockfd, buffer, 128);
    if(size  <= 0) //断开连接size=0
    {
        perror("接收错误，离线");
    }
    printf("%s\n", buffer);

    //4.关闭套接字close
    close(sockfd);

}