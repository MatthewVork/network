#include "ui.h"
#include "cJSON.h"
#include "../src/client.h"
#include <stdio.h>

// 注册屏幕上的按钮回调 
void User_register(lv_event_t * e) {
    // 明确从注册页面的文本框获取数据 
    const char * username = lv_textarea_get_text(ui_Username_register); 
    const char * password = lv_textarea_get_text(ui_Password_register);

    if (client_fd < 0 || strlen(username) == 0) {
        printf("Register failed: Connection lost or empty user.\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 1); // 注册业务 type 为 1 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    printf("Sent Register JSON: %s\n", json_out);
    
    cJSON_free(json_out);
    cJSON_Delete(root);
}

// 登录页面的登录按钮回调 
void User_login(lv_event_t * e) {
    const char * username = lv_textarea_get_text(ui_Username_login);
    const char * password = lv_textarea_get_text(ui_Password_login);

    if (client_fd < 0 || strlen(username) == 0) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 2); // 登录业务 type 为 2 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);
    printf("Sent Login JSON: %s\n", json_out);

    cJSON_free(json_out);
    cJSON_Delete(root);
}