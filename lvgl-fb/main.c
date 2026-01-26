#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"
#include "../src/client.h"

// --- 1. 手动鼠标驱动变量 ---
static int my_last_x = 0;
static int my_last_y = 0;
static bool my_is_pressed = false;

// --- 2. 手动鼠标回调 (让 LVGL 听我们的) ---
static void my_mouse_read(lv_indev_drv_t * drv, lv_indev_data_t * data) {
    data->point.x = my_last_x;
    data->point.y = my_last_y;
    data->state = my_is_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// --- 3. 辅助功能：自动隐藏提示框 ---
static void hide_ui_timer_cb(lv_timer_t * t) {
    lv_obj_t * obj = (lv_obj_t *)t->user_data;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t); 
}

// --- 4. 网络接收线程 (恢复 12 点版本逻辑) ---
void *network_recv_thread(void *arg) {
    char buf[2048];
    while (1) {
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            printf("[DEBUG] 收到消息: %s\n", buf);
            
            cJSON *root = cJSON_Parse(buf);
            if (!root) continue;

            cJSON *type = cJSON_GetObjectItem(root, "type");
            cJSON *st = cJSON_GetObjectItem(root, "status");
            cJSON *rid = cJSON_GetObjectItem(root, "room_id");

            if (type) g_player.msg_type = type->valueint;
            if (st) strncpy((char*)g_player.status, st->valuestring, 15);
            if (rid) g_player.room_id = rid->valueint;

            g_player.has_update = 1; // 通知主线程
            cJSON_Delete(root);
        }
        usleep(5000); // 避免空转占满CPU
    }
    return NULL;
}

// --- 5. LVGL 心跳 ---
static int tick_thread(void *data) {
    while(1) { SDL_Delay(5); lv_tick_inc(5); }
    return 0;
}

int main(int argc, char **argv) {
    // A. 基础初始化
    lv_init();
    sdl_init(); 

    // B. 显存配置
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

    // C. 【关键】注册手动鼠标驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_mouse_read; // 使用我们自己的读取函数
    lv_indev_drv_register(&indev_drv);

    // D. 启动线程
    SDL_CreateThread(tick_thread, "tick", NULL);
    
    // E. 恢复网络连接
    init_network(); 
    pthread_t tid;
    pthread_create(&tid, NULL, network_recv_thread, NULL);

    // F. 初始化 UI
    ui_init();

    printf("--- 系统完全启动：网络已连接，鼠标驱动正常 ---\n");

    // G. 主循环
    while (1) {
        // --- 核心：手动捕获事件并更新坐标 ---
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) return 0;
            
            if(event.type == SDL_MOUSEMOTION) {
                my_last_x = event.motion.x;
                my_last_y = event.motion.y;
            }
            else if(event.type == SDL_MOUSEBUTTONDOWN) {
                my_is_pressed = true;
            }
            else if(event.type == SDL_MOUSEBUTTONUP) {
                my_is_pressed = false;
            }
        }

        // --- 业务逻辑 (12点版本原样恢复) ---
        if (g_player.has_update) {
            // 注册反馈
            if (g_player.msg_type == 1) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_register_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_success);
                    _ui_screen_change(&ui_Screen_login, LV_SCR_LOAD_ANIM_FADE_ON, 500, 2000, &ui_Screen_login_screen_init);
                } else {
                    // 如果你有失败提示框，也可以在这里处理
                }
            } 
            // 登录反馈
            else if (g_player.msg_type == 2) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_login_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 1200, ui_Container_login_success);
                    _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 1000, &ui_Screen_main_menu_screen_init);
                }
            }
            // 创建/加入房间反馈
            else if (g_player.msg_type == 5 || g_player.msg_type == 7) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    _ui_screen_change(&ui_Screen_room, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_room_screen_init);
                    // 房间号显示 (如果 UI 里有这个 Label 且已初始化，下面这行才生效，否则也不报错)
                    // lv_label_set_text_fmt(ui_Label_roomnum, "%d", g_player.room_id); 
                }
            }
            g_player.has_update = 0; 
        }

        lv_timer_handler(); 
        usleep(5000); 
    }

    return 0;
}