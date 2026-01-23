#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"
#include "../src/client.h"

// --- 自动隐藏容器的定时器回调 ---
static void hide_ui_timer_cb(lv_timer_t * t) {
    lv_obj_t * obj = (lv_obj_t *)t->user_data;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t); 
}

// --- 子线程：负责从服务器接收并解析 JSON ---
void *network_recv_thread(void *arg) {
    char buf[2048];
    while (1) {
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            printf("[DEBUG] 收到原始回包: %s\n", buf); // 打印回码确认收到了
            
            cJSON *root = cJSON_Parse(buf);
            if (!root) continue;

            cJSON *type = cJSON_GetObjectItem(root, "type");
            cJSON *st = cJSON_GetObjectItem(root, "status");

            // 核心修复：如果服务器没给 type (Type=0)，我们根据 status 内容强制补齐逻辑
            if (type) {
                g_player.msg_type = type->valueint;
            } else if (st && strcmp(st->valuestring, "error") == 0) {
                // 特殊处理：如果收到 error 但没收到 type，强制设为 1 以触发注册/登录失败显示
                g_player.msg_type = 1; 
                printf("[FIX] 检测到回包缺少type，已手动修正标志位以显示UI\n");
            }

            if (st) strcpy((char*)g_player.status, st->valuestring);

            g_player.has_update = 1; 
            cJSON_Delete(root);
        }
    }
    return NULL;
}

static int tick_thread(void *data) {
    while(1) { SDL_Delay(5); lv_tick_inc(5); }
    return 0;
}

int main(int argc, char **argv) {
    lv_init();
    sdl_init();

    static lv_color_t buf[800 * 480];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 800 * 480);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);

    SDL_CreateThread(tick_thread, "tick", NULL);

    init_network(); // 初始化网络
    
    pthread_t tid;
    pthread_create(&tid, NULL, network_recv_thread, NULL);

    ui_init(); // 初始化界面

    while (1) {
        if (g_player.has_update) {
            printf("[DEBUG] 处理 UI 更新: Type=%d, Status=%s\n", g_player.msg_type, g_player.status);
            
            // 1. 处理注册反馈或强制补全的错误反馈 (Type 1)
            if (g_player.msg_type == 1) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_register_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_success);
                } 
                else {
                    // 这里就是你要的：去除隐藏标志
                    lv_obj_clear_flag(ui_Container_register_fail, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_fail);
                }
            } 
            // 2. 处理登录反馈 (Type 2)
            else if (g_player.msg_type == 2) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_login_success, LV_OBJ_FLAG_HIDDEN);
                    _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 1000, &ui_Screen_main_menu_screen_init);
                } else {
                    lv_obj_clear_flag(ui_Container_login_fail, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_login_fail);
                }
            }
            g_player.has_update = 0; 
        }
        lv_timer_handler(); 
        usleep(5000);
    }
    return 0;
}