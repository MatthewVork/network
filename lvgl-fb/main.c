#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"
#include "../src/client.h"

// --- 全局变量：用于手动驱动鼠标 ---
static int my_last_x = 0;
static int my_last_y = 0;
static bool my_is_pressed = false;

// --- 全局变量：用于跨线程传递准备状态 ---
static struct {
    int side;      // 谁更新了？1红 2黑
    int is_ready;  // 状态？0未准备 1已准备
    char opp_name[32]; // 对手名字
} sync_data;

// --- 手动鼠标驱动 ---
static void my_mouse_read(lv_indev_drv_t * drv, lv_indev_data_t * data) {
    data->point.x = my_last_x;
    data->point.y = my_last_y;
    data->state = my_is_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// --- 辅助：隐藏提示框 ---
static void hide_ui_timer_cb(lv_timer_t * t) {
    lv_obj_t * obj = (lv_obj_t *)t->user_data;
    if(obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t); 
}

// --- 网络线程 ---
void *network_recv_thread(void *arg) {
    char buf[2048];
    while (1) {
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            printf("[DEBUG] RECV: %s\n", buf);
            
            cJSON *root = cJSON_Parse(buf);
            if (!root) continue;

            cJSON *type = cJSON_GetObjectItem(root, "type");
            cJSON *st = cJSON_GetObjectItem(root, "status");
            cJSON *rid = cJSON_GetObjectItem(root, "room_id");

            if (type) g_player.msg_type = type->valueint;
            if (st) strncpy((char*)g_player.status, st->valuestring, 15);
            if (rid) g_player.room_id = rid->valueint;

            // 处理特殊业务数据
            if (g_player.msg_type == 12) { // 准备状态更新
                cJSON *sd = cJSON_GetObjectItem(root, "side");
                cJSON *rdy = cJSON_GetObjectItem(root, "is_ready");
                if (sd && rdy) {
                    sync_data.side = sd->valueint;
                    sync_data.is_ready = rdy->valueint;
                }
            }
            else if (g_player.msg_type == 8) { // 对手进入通知
                cJSON *opp = cJSON_GetObjectItem(root, "opp_name");
                if(opp) strncpy(sync_data.opp_name, opp->valuestring, 31);
            }

            g_player.has_update = 1; 
            cJSON_Delete(root);
        }
        usleep(5000); 
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

    // 显存 & 驱动初始化 (保持你之前的代码)
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
    indev_drv.read_cb = my_mouse_read; // 使用手动驱动
    lv_indev_drv_register(&indev_drv);

    SDL_CreateThread(tick_thread, "tick", NULL);
    init_network(); 
    pthread_t tid;
    pthread_create(&tid, NULL, network_recv_thread, NULL);

    ui_init();

    // ============================================
    // 【关键修复】手动绑定准备按钮的事件
    // 因为 ui.c 里没有绑定，我们在这里补上
    // ============================================
    if (ui_Button1) {
        lv_obj_add_event_cb(ui_Button1, Ready_Handler, LV_EVENT_CLICKED, NULL);
        printf("[SYSTEM] 准备按钮事件已手动绑定\n");
    }

    printf("--- 系统就绪 ---\n");

    while (1) {
        // 1. 手动事件泵
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) return 0;
            if(event.type == SDL_MOUSEMOTION) {
                my_last_x = event.motion.x;
                my_last_y = event.motion.y;
            } else if(event.type == SDL_MOUSEBUTTONDOWN) {
                my_is_pressed = true;
            } else if(event.type == SDL_MOUSEBUTTONUP) {
                my_is_pressed = false;
            }
        }

        // 2. 业务逻辑处理
        if (g_player.has_update) {
            
            // --- 注册 & 登录 ---
            if (g_player.msg_type == 1) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_register_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 2000, ui_Container_register_success);
                    _ui_screen_change(&ui_Screen_login, LV_SCR_LOAD_ANIM_FADE_ON, 500, 1500, &ui_Screen_login_screen_init);
                }
            } 
            else if (g_player.msg_type == 2) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_login_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 1000, ui_Container_login_success);
                    _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 500, &ui_Screen_main_menu_screen_init);
                }
            }
            
            // --- 创建/加入房间成功 ---
            else if (g_player.msg_type == 5 || g_player.msg_type == 7) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    _ui_screen_change(&ui_Screen_room, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_room_screen_init);
                    
                    // A. 显示房间号 (假设 ui_Label1 是房间标题)
                    if (ui_Label1) lv_label_set_text_fmt(ui_Label1, "Room: %d", g_player.room_id);

                    // B. 显示自己的名字
                    if (g_player.side == 1 && ui_Label_red_player_username) // 红方(房主)
                        lv_label_set_text(ui_Label_red_player_username, (char*)g_player.username);
                    else if (g_player.side == 2 && ui_Label_black_player_username) // 黑方(加入者)
                        lv_label_set_text(ui_Label_black_player_username, (char*)g_player.username);
                }
            }

            // --- 对手进入 (Type 8) ---
            else if (g_player.msg_type == 8) {
                // 如果我是红方，进来的肯定是黑方
                if (g_player.side == 1 && ui_Label_black_player_username) {
                    lv_label_set_text(ui_Label_black_player_username, sync_data.opp_name);
                }
            }

            // --- 准备状态变更 (Type 12) ---
            else if (g_player.msg_type == 12) {
                lv_obj_t * label = NULL;
                // 根据 side 决定更新哪个 Label
                if (sync_data.side == 1) label = ui_Label_red_player_ready_info;
                else label = ui_Label_black_player_ready_info;

                if (label) {
                    if (sync_data.is_ready) {
                        lv_label_set_text(label, "READY");
                        lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0); // 绿色
                    } else {
                        lv_label_set_text(label, "WAIT");
                        lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0); // 红色
                    }
                }
            }
            // --- 退出房间 (Type 9) ---
            else if (g_player.msg_type == 9) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                     _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_main_menu_screen_init);
                }
            }

            g_player.has_update = 0; 
        }

        lv_timer_handler(); 
        usleep(5000); 
    }
    return 0;
}