#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"
#include "../src/client.h"

// --- 子线程：负责从服务器接收数据并“翻译”成结构体 ---
void *network_recv_thread(void *arg) {
    char buf[2048];
    while (1) {
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            cJSON *root = cJSON_Parse(buf);
            if (!root) continue;

            // 填充全局结构体
            cJSON *type = cJSON_GetObjectItem(root, "type");
            if (type) g_player.msg_type = type->valueint;

            cJSON *st = cJSON_GetObjectItem(root, "status");
            if (st) strcpy((char*)g_player.status, st->valuestring);

            cJSON *msg = cJSON_GetObjectItem(root, "msg");
            if (msg) strcpy((char*)g_player.msg_text, msg->valuestring);

            // 吹哨子通知主线程
            g_player.has_update = 1; 
            cJSON_Delete(root);
        }
    }
    return NULL;
}

// 隐藏容器的回调
static void hide_ui_timer_cb(lv_timer_t * t) {
    lv_obj_t * obj = (lv_obj_t *)t->user_data;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
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

    init_network();
    
    pthread_t tid;
    pthread_create(&tid, NULL, network_recv_thread, NULL);

    ui_init();

    while (1) {
        if (g_player.has_update) {
            // 根据 msg_type 区分处理：1 是注册反馈，2 是登录反馈 
            if (g_player.msg_type == 1) { 
                // 处理注册结果 
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_register_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_success);
                } else {
                    lv_obj_clear_flag(ui_Container_register_fail, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_fail);
                }
            } 
            else if (g_player.msg_type == 2) {
                // 处理登录结果 
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_login_success, LV_OBJ_FLAG_HIDDEN);
                    // 登录成功通常跳转主菜单 
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