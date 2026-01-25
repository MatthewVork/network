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
}

void event_button_exit(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "type", 6); 
        cJSON_AddNumberToObject(root, "room_id", g_player.room_id);
        cJSON_AddStringToObject(root, "username", (char*)g_player.username);

        char *json_out = cJSON_PrintUnformatted(root);
        send(client_fd, json_out, strlen(json_out), 0);
        
        g_player.room_id = 0; 
        g_player.side = 0;    

        _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_main_menu_screen_init);

        cJSON_free(json_out);
        cJSON_Delete(root);
    }
}