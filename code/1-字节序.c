#include <stdio.h>
#include <arpa/inet.h>
int main(void)
{
    unsigned int data = 0x12345678;//主机字节序
    unsigned int ndata = htonl(data);//转网络字节序
    printf("%x---%x\n", data, ndata);
    return 0;
}

/*
网络编程中一般采用大端序(高位低字节)，多字节才考虑字节序

htonl   4字节数据，主机转网络 (long-8(64), long-4(32))
htons   2字节数据，主机转网络

ntohl   4字节数据，网络转主机
ntohs   2字节数据，网络转主机
*/