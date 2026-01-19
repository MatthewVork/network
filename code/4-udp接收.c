#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
int main(void)
{
    //创建套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    //绑定
    struct  sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));//清空
    addr.sin_family = AF_INET;//初始化协议族
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret  = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        perror("bind");
        return -1;
    }

    //接收数据
    char buffer[128]={0};
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int size =  recvfrom(sockfd, buffer, 128, 0,(struct sockaddr*)&caddr/*保存发送端地址*/, &len);
    if(size < 0)
    {
        perror("recvfrom");
    }
    printf("%s-%d-%s\n",inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), buffer);

    //关闭套接字
    close(sockfd);
    return 0;
}