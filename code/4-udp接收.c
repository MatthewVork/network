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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   //绑定内地址
    int ret  = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        perror("bind");
        return -1;
    }

    /*
    
    绑定函数

    ssize_t bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    返回值：成功 -> 0 失败 -> -1

    参数:   int sockfd 套接字
            const struct sockaddr *addr 地址
            socklen_t addrlen 地址长度

    */

    //接收数据
    char buffer[128]={0};
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int size =  recvfrom(sockfd, buffer, 128, 0,(struct sockaddr*)&caddr/*保存发送端地址*/, &len);
    if(size < 0)
    {
        perror("recvfrom");
    }

    /*
    
    接收函数
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    返回值：成功 -> 接受数据大小 失败 -> -1
    
    参数  int sockfd 套接字
          void *buf 储存数据的首地址
          size_t len 储存数据的空间大小
          int flag 标志位，一般为0
          struct sockaddr *src_addr 保存发送方的地址
          socklen_t *addrlen 地址长度

    */
    printf("%s-%d-%s\n",inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), buffer);

    //关闭套接字
    close(sockfd);
    return 0;
}