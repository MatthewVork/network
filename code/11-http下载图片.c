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
    saddr.sin_port = htons(80);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int ret = connect(sockfd, (struct sockaddr*)&saddr,sizeof(saddr));
    if(ret < 0)
    {
        perror("connect");
        return -1;
    }
    //打开http请求头
    const char* request = "GET /1.png HTTP/1.1\r\nhost:127.0.0.1\r\n\r\n";
    write(sockfd, request, strlen(request));


    FILE *file = fopen("1.png", "w");

    //接收数据
    char buffer[1024];
    int size = read(sockfd, buffer, 1024);
    if(size  <= 0) //断开连接size=0
    {
        perror("接收错误，离线");
    }
    //找协议头结束位置， 找到数据长度
    char *pic = strstr(buffer, "\r\n\r\n");
    if(pic){
        pic+=4;
    }else {return -1;}
    int filesize = 0;
    char *p = strstr(buffer, "Content-Length: ");
    if(p)
    {
        sscanf(p,"Content-Length: %d", &filesize);
    }

    //把图片数据写入文件--随着头发送过来的图片数据大小
    int headsize = pic-buffer;
    int wsize = fwrite(pic, 1, size-headsize, file);
    printf("filesize:%d, headsize:%d, wsize:%d, size:%d\n", filesize, headsize, wsize,size);
    
    while(wsize < filesize)
    {
        int size = read(sockfd, buffer, 1024);
        if(size  <= 0) //断开连接size=0
        {
            perror("接收错误，离线");
        }
        int ret = fwrite(buffer, 1,size, file);
        wsize += ret;
    }
    fclose(file);

    //4.关闭套接字close
    close(sockfd);
}