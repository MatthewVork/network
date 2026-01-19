#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
void* client_run(void* arg);

int clients[100]={0};
int n=0;
int main(void)
{
    //1.创建套接字，2.绑定，3.监听,4.接受连接， 5.收发数据， 6.关闭
    //1.创建套接字socket， 
    int sockfd = socket(AF_INET, SOCK_STREAM,0);
    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }
    //2.绑定
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret= bind(sockfd, (struct sockaddr *)&saddr,sizeof(saddr));
    if(ret < 0)
    {
        perror("bind");
        return -1;
    }

    //3.监听
    ret =  listen(sockfd, 5);
    if(ret < 0)
    {
        perror("listen");
        return -1;
    }

    //4.接受连接
    while(1){
        struct sockaddr_in caddr;
        socklen_t len = sizeof(caddr);
        int clientfd =  accept(sockfd, (struct sockaddr *)&caddr,&len);
        if(clientfd < 0)
        {
            perror("accept");
            continue;
        }
        //发送数据
        printf("客户端:%s\n", inet_ntoa(caddr.sin_addr));
        char tmp[32];
        sprintf(tmp, "client:%d", clientfd);
        write(clientfd, tmp,strlen(tmp));

        //创建线程，在线程中读取客户端数据（每一个客户端创建一个线程）
        pthread_t id = 0;
        pthread_create(&id, NULL, client_run, (void*)clientfd);
        pthread_detach(id);

        //保存客户端套接字
        for(int i=0;i<100; i++)
        {
            if(clients[i] == 0)
            {
                clients[i] = clientfd;
                break;
            }
        }
    }

}

void* client_run(void* arg)
{
    int clientfd = (int)arg;
    while(1)
    {
        //接收数据
        char buffer[128]={0};
        int size = read(clientfd, buffer, 128);
        if(size <= 0)
        {
            printf("客户端离线\n");
            for(int i=0;i<100; i++)
            {
                if(clients[i] == clientfd)
                {
                    clients[i] = 0;
                    break;
                }
            }
            break;
        }
        printf("%s\n", buffer);
        for(int i=0; i<100;i++)
        {
            if(clients[i] != 0 && clients[i] !=clientfd)
            {
                write(clients[i] , buffer, size);
            }
        }

    }
    close(clientfd);
    return NULL;
}