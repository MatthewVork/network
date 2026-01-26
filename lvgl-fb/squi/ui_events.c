#include "ui.h"
#include "cJSON.h"
#include "../src/client.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

extern int client_fd;
extern volatile PlayerGlobalData g_player;

// --- 键盘安全隐藏工具 ---
static void timer_hide_kb_cb(lv_timer_t * t) {
    lv_obj_t * kb = (lv_obj_t *)t->user_data;
    if(kb && lv_obj_is_valid(kb)) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_del(t); 
}
void Safe_Close_KB(lv_obj_t * kb) {
    if (kb && lv_obj_is_valid(kb)) {
        lv_timer_create(timer_hide_kb_cb, 20, kb);
    }
}

void Text_clean(lv_event_t * e) {
    // 这里的逻辑先不动
    lv_obj_t * target = lv_event_get_target(e);
    if(target == ui_Screen_login) {
        lv_textarea_set_text(ui_Username_login, "");
        lv_textarea_set_text(ui_Password_login, "");
    }
}

// =================================================
// 核心诊断函数：User_login
// =================================================
void User_login(lv_event_t * e) {
    printf("👉 [Step 1] 进入 User_login 函数...\n");

    // 1. 尝试获取输入框文本
    printf("👉 [Step 2] 正在获取用户名...\n");
    const char * username = lv_textarea_get_text(ui_Username_login);
    if (username == NULL) {
        printf("❌ [Error] 用户名指针为 NULL！\n");
        return;
    }
    printf("✅ 用户名: %s\n", username);

    printf("👉 [Step 3] 正在获取密码...\n");
    const char * password = lv_textarea_get_text(ui_Password_login);
    if (password == NULL) {
        printf("❌ [Error] 密码指针为 NULL！\n");
        return;
    }
    
    // 2. 隐藏键盘
    printf("👉 [Step 4] 准备隐藏键盘...\n");
    Safe_Close_KB(ui_Keyboard_login);

    // 3. 检查连接
    printf("👉 [Step 5] 检查网络连接 FD=%d...\n", client_fd);
    if (client_fd < 0) {
        printf("❌ [Error] 未连接到服务器！\n");
        return;
    }

    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("⚠️ [Info] 输入为空，不发送。\n");
        return;
    }

    // 4. 构造 JSON
    printf("👉 [Step 6] 正在构造 JSON...\n");
    strncpy((char*)g_player.username, username, 31);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 2); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);

    char *json_out = cJSON_PrintUnformatted(root);
    printf("✅ JSON 内容: %s\n", json_out);

    // 5. 发送
    printf("👉 [Step 7] 准备发送 send()...\n");
    
    // 使用 MSG_NOSIGNAL | MSG_DONTWAIT 防止卡死
    int ret = send(client_fd, json_out, strlen(json_out), MSG_NOSIGNAL | MSG_DONTWAIT);
    
    if (ret < 0) {
        printf("❌ [Error] 发送失败! errno=%d, 原因: %s\n", errno, strerror(errno));
        // 如果是 EPIPE (32)，说明服务器断了
    } else {
        printf("🎉 [Step 8] 发送成功！发送了 %d 字节\n", ret);
    }

    cJSON_free(json_out);
    cJSON_Delete(root);
    printf("👉 [Step 9] 函数执行完毕，准备退出 User_login\n");
}

// -------------------------------------------------
// 其他函数保持原样，或者也加上类似的 printf
// -------------------------------------------------
void User_register(lv_event_t * e) {
    printf("👉 [Register] 进入注册函数...\n");
    Safe_Close_KB(ui_Keyboard_register);
    const char * username = lv_textarea_get_text(ui_Username_register); 
    const char * password = lv_textarea_get_text(ui_Password_register);

    if (client_fd < 0) {
        printf("❌ 未连接\n"); 
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", 1); 
    cJSON_AddStringToObject(root, "user", username);
    cJSON_AddStringToObject(root, "pass", password);
    char *json_out = cJSON_PrintUnformatted(root);
    
    printf("👉 [Register] 发送中...\n");
    send(client_fd, json_out, strlen(json_out), MSG_NOSIGNAL | MSG_DONTWAIT);
    printf("👉 [Register] 发送完成\n");
    
    cJSON_free(json_out);
    cJSON_Delete(root);
}
// ... 其他函数 (Join_Room_Handler 等) 照旧 ...
// 必须保留 Delay_Hide_Keyboard，否则报错
void Delay_Hide_Keyboard(lv_event_t * e) {
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t * kb_to_hide = NULL;
    if (target == ui_Username_login || target == ui_Password_login) kb_to_hide = ui_Keyboard_login;
    else if (target == ui_Username_register || target == ui_Password_register) kb_to_hide = ui_Keyboard_register;
    else if (target == ui_TextArea_roomnum) kb_to_hide = ui_Keyboard_roomnum;
    Safe_Close_KB(kb_to_hide);
}
void Create_Room_Handler(lv_event_t * e) {} 
void Join_Room_Handler(lv_event_t * e) {}
void Ready_Handler(lv_event_t * e) {}
void event_button_exit(lv_event_t * e) {}
void event_button_logout(lv_event_t * e) {}