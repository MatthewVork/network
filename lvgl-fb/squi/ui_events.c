#include "ui.h"
#include "cJSON.h"
#include "../src/client.h"
#include <stdio.h>
#include <string.h>

extern int client_fd;
extern volatile PlayerGlobalData g_player;

void User_register(lv_event_t * e) {
    const char * username = lv_textarea_get_text(ui_Username_register); 
    const char * password = lv_textarea_get_text(ui_Password_register);
    if (client_fd < 0 || strlen(username) == 0) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 1); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    cJSON_free(json_out);
    cJSON_Delete(root);
}

void User_login(lv_event_t * e) {
    const char * username = lv_textarea_get_text(ui_Username_login);
    const char * password = lv_textarea_get_text(ui_Password_login);
    if (client_fd < 0 || strlen(username) == 0) return;

    strncpy((char*)g_player.username, username, 31);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 2); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    cJSON_free(json_out);
    cJSON_Delete(root);
}

void Create_Room_Handler(lv_event_t * e) {
    if (client_fd < 0) return;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 5); 
    cJSON_AddStringToObject(root, "user", (char*)g_player.username);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    cJSON_free(json_out);
    cJSON_Delete(root);
}

void Text_clean(lv_event_t * e) {
    lv_obj_t * target = lv_event_get_target(e);
    if(target == ui_Screen_login) {
        lv_textarea_set_text(ui_Username_login, "");
        lv_textarea_set_text(ui_Password_login, "");
    }
    if(target == ui_Screen_register) {
        lv_textarea_set_text(ui_Username_register, "");
        lv_textarea_set_text(ui_Password_register, "");
    }
    if(target == ui_Button_cancel_input_roomnum) {
        lv_textarea_set_text(ui_TextArea_roomnum, "");
    }
}

void event_button_exit(lv_event_t * e)
{
    // 1. 构造退出房间的请求
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 9); // Type 9 对应服务器的 handle_leave_room
    cJSON_AddStringToObject(root, "user", (char*)g_player.username);
    cJSON_AddNumberToObject(root, "room_id", g_player.room_id);

    // 2. 序列化并发送
    char *out = cJSON_PrintUnformatted(root);
    if (client_fd != -1) {
        send(client_fd, out, strlen(out), 0);
        printf("[CLIENT] 已发送退出房间请求: %s\n", out);
    }

    // 3. 释放资源
    cJSON_free(out);
    cJSON_Delete(root);

    /* 注意：这里不需要手动调用 _ui_screen_change。
       我们应该等服务器回传 Type 9 的 "success" 包，
       由 main.c 里的循环统一处理屏幕跳转。
    */
}

void event_button_logout(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        // 1. 构造退出消息发送给服务器
        if (client_fd >= 0) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "type", 6); // 定义 6 为注销/退出登录
            cJSON_AddStringToObject(root, "user", (char*)g_player.username);
            
            char *json_out = cJSON_PrintUnformatted(root);
            send(client_fd, json_out, strlen(json_out), 0);
            
            cJSON_free(json_out);
            cJSON_Delete(root);
            printf("[CLIENT] 已向服务端发送退出请求\n");
        }

        // 2. 清除本地全局变量，恢复初始状态
        memset((void*)&g_player, 0, sizeof(PlayerGlobalData));
        g_player.has_update = 0;
        
        printf("[CLIENT] 本地登录状态已清除，正在返回登录界面\n");
    }
}

// ui_events.c
void Join_Room_Handler(lv_event_t * e)
{
    // 1. 从你的 SLS 组件中获取输入的房间号
    const char * id_str = lv_textarea_get_text(ui_TextArea_roomnum); 
    int target_id = atoi(id_str); // 转成整数

    if (client_fd < 0 || target_id <= 0) return;

    // 2. 封装 JSON 请求 (Type 7)
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 7);
    cJSON_AddStringToObject(root, "user", (char*)g_player.username);
    cJSON_AddNumberToObject(root, "room_id", target_id);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    
    cJSON_free(json_out);
    cJSON_Delete(root);
    printf("[CLIENT] 正在申请加入房间: %d\n", target_id);
}

void Ready_Handler(lv_event_t * e) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 11); // 11: 准备状态切换
    cJSON_AddStringToObject(root, "user", (char*)g_player.username);
    cJSON_AddNumberToObject(root, "room_id", g_player.room_id);

    char *out = cJSON_PrintUnformatted(root);
    send(client_fd, out, strlen(out), 0);
    cJSON_free(out);
    cJSON_Delete(root);
}
