#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "cJSON.h"

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
    saddr.sin_addr.s_addr = inet_addr("116.62.81.138");
    int ret = connect(sockfd, (struct sockaddr*)&saddr,sizeof(saddr));
    if(ret < 0)
    {
        perror("connect");
        return -1;
    }    

    //打包请求命令
    const char *req = "GET /v3/weather/now.json?key=SbslwZ6X47ih3u-bX&location=beijing&language=zh-Hans&unit=c\r\nHTTP/1.1\r\nhost:api.seniverse.com\r\n\r\n";
    int wsize = write(sockfd, req, strlen(req));   
    if(wsize < 0)
    {
        perror("request");
        return -1;
    }

    //接收应答数据
    char buffer[1024]={0};
    int rsize = read(sockfd, buffer, 1024);
    if(rsize <= 0)
    {
        perror("read");
        return -1;
    }
    printf("%s\n", buffer);

    //把字符串转cJSON结构体
    cJSON* root = cJSON_Parse(buffer);
    if(root)
    {
        cJSON* array = cJSON_GetObjectItem(root, "results");
        if(array)
        {
            //获取数组中的第一个元素
            cJSON* arrayObj = cJSON_GetArrayItem(array, 0);
            if(arrayObj)
            {
                cJSON* nowObj = cJSON_GetObjectItem(arrayObj, "now");
                if(nowObj)
                {
                   printf("天气:%s\n", cJSON_GetObjectItem(nowObj,"text")->valuestring); 
                   printf("温度:%s\n",cJSON_GetObjectItem(nowObj,"temperature")->valuestring);
                }
            }
        }
    }
    cJSON_Delete(root);

    close(sockfd);
    return 0;
}