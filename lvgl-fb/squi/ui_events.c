#include "ui.h"
#include "cJSON.h"
#include "../src/client.h"

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
    // 发完就走，不管结果，结果由 main.c 的子线程去收
}

void User_login(lv_event_t * e) {
    const char * username = lv_textarea_get_text(ui_Username_login);
    const char * password = lv_textarea_get_text(ui_Password_login);

    if (client_fd < 0 || strlen(username) == 0) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 2); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    send(client_fd, json_out, strlen(json_out), 0);

    cJSON_free(json_out);
    cJSON_Delete(root);
}