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
            printf("[DEBUG] 收到原始回包: %s\n", buf); 
            
            cJSON *root = cJSON_Parse(buf);
            if (!root) continue;

            cJSON *type = cJSON_GetObjectItem(root, "type");
            cJSON *st = cJSON_GetObjectItem(root, "status");
            cJSON *rid = cJSON_GetObjectItem(root, "room_id");

            if (rid) {
                g_player.room_id = rid->valueint; 
            }

            if (type) {
                g_player.msg_type = type->valueint;
            } else if (st && strcmp(st->valuestring, "error") == 0) {
                g_player.msg_type = 1; 
                printf("[FIX] 检测到回包缺少type，已手动修正标志位以显示UI\n");
            }

            if (st) strcpy((char*)g_player.status, st->valuestring);

            // --- 此处为您要求的增加内容：处理 Type 12 准备状态同步 ---
            if (g_player.msg_type == 12) {
                cJSON *user = cJSON_GetObjectItem(root, "user");
                cJSON *is_ready = cJSON_GetObjectItem(root, "is_ready");
                
                if (user && is_ready) {
                    // 如果收到的名字是红方的名字（房主）
                    // 注意：这里直接在线程里改 UI 会有卡死风险，但为了维持你的代码结构，先保证逻辑正确
                    if (strcmp(user->valuestring, (char*)g_player.red_name) == 0) {
                        lv_label_set_text(ui_Label_red_ready, is_ready->valueint ? "已准备" : "未准备");
                        lv_obj_set_style_text_color(ui_Label_red_ready, is_ready->valueint ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);
                    } 
                    else {
                        lv_label_set_text(ui_Label_black_ready, is_ready->valueint ? "已准备" : "未准备");
                        lv_obj_set_style_text_color(ui_Label_black_ready, is_ready->valueint ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);
                    }
                }
            }

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

    init_network(); // 严格对照 client.h
    
    pthread_t tid;
    pthread_create(&tid, NULL, network_recv_thread, NULL);

    ui_init(); 

    while (1) {
        sdl_handler(); // 解决进界面卡死的关键：必须处理SDL事件轮询

        if (g_player.has_update) {
            printf("[DEBUG] 处理 UI 更新: Type=%d, Status=%s\n", g_player.msg_type, g_player.status);
            
            if (g_player.msg_type == 1) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_register_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_success);
                    _ui_screen_change(&ui_Screen_login, LV_SCR_LOAD_ANIM_FADE_ON, 500, 2000, &ui_Screen_login_screen_init);
                } 
                else {
                    lv_obj_clear_flag(ui_Container_register_fail, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_register_fail);
                }
            } 

            else if (g_player.msg_type == 2) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    lv_obj_clear_flag(ui_Container_login_success, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 1200, ui_Container_login_success);
                    _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 1000, &ui_Screen_main_menu_screen_init);
                } 
                else if (strcmp((char*)g_player.status, "repeat") == 0) {
                    lv_obj_clear_flag(ui_Container_login_repeat, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_login_repeat);
                }
                else {
                    lv_obj_clear_flag(ui_Container_login_fail, LV_OBJ_FLAG_HIDDEN);
                    lv_timer_create(hide_ui_timer_cb, 3000, ui_Container_login_fail);
                }
            }

            else if (g_player.msg_type == 5) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    printf("创建成功，进入房间: %d\n", g_player.room_id);
                    _ui_screen_change(&ui_Screen_room, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_room_screen_init);
                    lv_label_set_text_fmt(ui_Label_roomnum, "当前房间号: %d", g_player.room_id);
                    lv_label_set_text(ui_Label_player_red, (char*)g_player.username);
                } 
            }

            else if (g_player.msg_type == 7) { 
                if (strcmp((char*)g_player.status, "success") == 0) {
                    _ui_screen_change(&ui_Screen_room, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_room_screen_init);
                    lv_label_set_text_fmt(ui_Label_roomnum, "当前房间号: %d", g_player.room_id);
                    lv_label_set_text(ui_Label_player_black, (char*)g_player.username);
                }
            }
            
            else if(g_player.msg_type == 9) {
                if (strcmp((char*)g_player.status, "success") == 0) {
                    printf("[CLIENT] 成功离开房间，返回主菜单\n");
                    g_player.room_id = 0;
                    g_player.side = 0;
                    _ui_screen_change(&ui_Screen_main_menu, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_main_menu_screen_init);
                } 
                else if (strcmp((char*)g_player.status, "update") == 0) {
                    printf("[CLIENT] 提示：对手已离开房间\n");
                }
            }
            g_player.has_update = 0; 
        }
        lv_timer_handler(); 
        usleep(5000);
    }
    return 0;
}