#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cjson/cJSON.h"

//设计一个链表存储客户端ID和套接字,状态
struct  UserInfo{
    struct UserInfo* next;
    struct UserInfo* prev;
    char ID[32]; //保存客户端ID
    int sockfd; //保存与客户端连接的套接字
    int status; //在线状态在线-1， 离线-0
};
//创建节点函数
struct UserInfo* create_node(char *ID, int fd, int status)
{
    //创建节点
    struct UserInfo* node = malloc(sizeof(struct UserInfo));
    node->prev = node; 
    node->next = node;
    node->sockfd = fd;
    node->status = status;
    strcpy(node->ID, ID);
    return node;
}
//查找节点函数（ID）（套接字）
struct UserInfo* find_id(struct UserInfo* head, char *ID)
{
    struct UserInfo* p = head->next;
    while(p != head)
    {
        if(strcmp(p->ID, ID) == 0)
        {
            return p;
        }
        p = p->next;
    }
    return  NULL;
}
//查找节点函数（套接字）
struct UserInfo* find_fd(struct UserInfo* head, int fd)
{
    struct UserInfo* p = head->next;
    while(p != head)
    {
        if(p->sockfd==fd)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}
//删除节点函数
bool  delete_node(struct UserInfo *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
    free(node);
    return true;
}
//添加节点函数
bool insert_node(struct UserInfo* head, struct UserInfo* node)
{
    head = head->prev;
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
    return true;
}


//客户端线程
void* client_run(void* arg);

//创建一个链表头指针（保存所有的客户端信息）
static struct UserInfo* head = NULL;

int main(void)
{
    //初始化链表头
    head = create_node("", 0, 0);
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

        //创建线程，在线程中读取客户端数据（每一个客户端创建一个线程）
        pthread_t id = 0;
        pthread_create(&id, NULL, client_run, (void*)clientfd);
        pthread_detach(id);
    }

}
//解析json数据
//1解析type， status
int parse_type_status(const char *buffer, const char *key)
{
    int num = -1;
    //解析数据--json
    cJSON* root = cJSON_Parse(buffer);
    if(root != NULL)
    {
        cJSON* item = cJSON_GetObjectItem(root,key);
        if(item != NULL)
        {
            num = item->valueint;
        }
    }
    cJSON_Delete(root);
    return num;
}
//2.解析ID,data
bool parse_id_data(const char *buffer, const char *key, char *value, int size)
{
    bool flag = false;
    //解析数据--json
    cJSON* root = cJSON_Parse(buffer);
    if(root != NULL)
    {
        cJSON* item = cJSON_GetObjectItem(root,key);
        if(item != NULL)
        {
            strncpy(value, item->valuestring, size);
            flag = true;
        }
    }
    cJSON_Delete(root);
    return flag;
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
            //通过套接字查找到客户端节点修改状态
            struct UserInfo* node = find_fd(head, clientfd);
            if(node != NULL)
            {
                node->status = 0;
                node->sockfd = 0;
            }
            break;
        }
        printf("%s\n", buffer);
        int type = parse_type_status(buffer, "type");
        if(type == 0)
        {
            //登录指令
            char  id[32] = {0};
            if(parse_id_data(buffer, "ID", id, 32))
            {
                //把客户端添加到链表---1.客户端以前是否登录过
                struct UserInfo* node = find_id(head, id);
                if(node == NULL)
                {
                    //以前没有登录过
                    node = create_node(id, clientfd, 1);
                    insert_node(head, node);
                }else
                {
                    //已经过登录过就直接修改状态，和套接字
                    node->sockfd = clientfd;
                    node->status = 1; 
                }
            }

        }else if(type == 1)
        {
            //转发数据指令
            char data[128] = {0};
            char id [32] = {0};
            parse_id_data(buffer, "ID", id, 32);
            //从链表中查找id对于的节点
            struct UserInfo* node = find_id(head, id);
            if(node != NULL)
            {
                int ret = write(node->sockfd, buffer, strlen(buffer));
                if(ret == strlen(data))
                {
                    char rep[] = "{\"type\":2,\"status\":0}";
                    write(clientfd, rep, strlen(rep));
                } 
            }
        }
    }
    close(clientfd);
    return NULL;
}