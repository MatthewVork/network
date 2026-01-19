#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if 0
typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
};
#endif

int main(void)
{
    //1.把字符串的IP地址转网络字节序的IP（32位整型数）
    char *ip = "192.168.63.1";
    in_addr_t ip32 =  inet_addr(ip);
    printf("%x\n", ip32);   //%x：16进制输出 

    //2.把32位网络字节序转字符串ip地址 
    struct in_addr in = {ip32};
    char *cip = inet_ntoa(in);  //该函数负责转换
    printf("%s\n", cip);
}

/*
inet_pton 字符串 -> 二进制 IPV4/IPV6
inet_ntop 二进制 -> 字符串 IPV4/IPV6
inet_addr 字符串 -> 二进制 IPV4
inet_aton 字符串 -> 二进制 IPV4
inet_ntoa 二进制 -> 字符串 IPV4
*/