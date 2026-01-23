#include "ui.h"
#include "cJSON.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

extern int client_fd; 

// --- 注册按钮事件 ---
void User_register(lv_event_t * e)
{
    // 【已确认】使用 ui.h 中的注册专用变量名
    const char * username = lv_textarea_get_text(ui_Username_register); 
    const char * password = lv_textarea_get_text(ui_Password_register);

    if (client_fd < 0 || strlen(username) == 0) {
        printf("用户名不能为空或网络未连接\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 1); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    printf("【客户端】发送注册请求: %s\n", json_out);

    cJSON_free(json_out);
    cJSON_Delete(root);
}

// --- 登录按钮事件 ---
void User_login(lv_event_t * e)
{
    // 【已确认】使用 ui.h 中的登录专用变量名
    const char * username = lv_textarea_get_text(ui_Username_login);
    const char * password = lv_textarea_get_text(ui_Password_login);

    if (client_fd < 0 || strlen(username) == 0) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 2); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    printf("【客户端】发送登录请求: %s\n", json_out);

    cJSON_free(json_out);
    cJSON_Delete(root);
}