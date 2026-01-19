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

    //开启广播
    int on = 1;
    int sret  = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,&on, sizeof(on));
    if(sret < 0)
    {
        perror("setsockopt");
        return -1;
    }


    //发送数据
    char buffer[]="hello world";
    struct  sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));//清空
    addr.sin_family = AF_INET;//初始化协议族
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = inet_addr("172.31.207.255");
    int size = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
    if(size < 0)
    {
        perror("sendto");
    }

    //关闭套接字
    close(sockfd);
    return 0;
}

/*
udp属于传输层协议
缺点：非连接，不可靠(协议无法确认数据已经到达对方)
优点：速度快，实时性强

应用场景：实时性要求高，数据安全性要求低(视频，音频)

---udp协议数据无序---
(接受数据的时候需要标上标号，接收数据的时候重组)

发送端：创建套接字 -> --- -> 发送数据
接收端：创建套接字 -> 绑定 -> 接受数据

所需库函数：type.h socket.h

int socket(int domain, int type, int protocol); 
返回成功 -> 返回套接字符 失败 -> -1

domain -> 协议(IPV4/IPV6) AF_INET，AF_INET6
type数据类型 SOCK_STREAM -> TCP协议 SOCK_DGRAM -> UDP协议 SOCK -> 原始套接字(需要自定义底层协议)

若使用数据流可能会导致粘包(Sticky Packets)

现象：发送方发送了两条 JSON 数据 {"type":0} 和 {"type":1}，但接收方可能一次性读到了 {"type":0}{"type":1}。
原因：TCP 是面向流的，它不保留消息边界。为了提高效率，TCP 可能会把几个小包合并成一个大包发送（Nagle 算法）。
后果：你的 cJSON_Parse 会解析失败，因为它不知道从哪里结束第一条数据。

protocol 协议 (默认0) 会根据前面的类型来自动选择相应的协议

---发送数据---
sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *desk_addr, socklen_t addrlen);
返回成功 -> 发送字节数 返回失败 -> -1

参数：
int sockfd                          套接字
const void                          *buf 发送的数据首地址
size_t len                          发送的数据长度
int flags                           默认为0
const struct sockaddr *dest_addr    发送的目标地址
socklen_t addrlen                   目标地址长度

实际使用中，通常使用 struct sockaddr_in 代替 struct sockaddr 来进行初始化协议，端口号，IP地址

struct sockaddr_in {
    short int          sin_family;  // 地址族 (Address Family)，通常设为 AF_INET
    unsigned short int sin_port;    // 16位端口号 (使用网络字节序 htons)
    struct in_addr     sin_addr;    // IPv4 地址结构体
    unsigned char      sin_zero[8]; // 填充 0，为了让此结构体与 struct sockaddr 大小一致
};

struct in_addr {
    uint32_t s_addr; // 32位 IPv4 地址 (使用网络字节序 inet_addr/inet_pton)
};

*/