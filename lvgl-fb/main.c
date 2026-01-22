#include "lvgl/lvgl.h"          // 必须放在最顶端
#include <SDL2/SDL.h>
#include <unistd.h>
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"                 // 确保 squi 目录在 CMake 包含路径中

// LVGL 心跳线程：每 5ms 告知 LVGL 时间流逝
static int tick_thread(void *data) {
    (void)data;
    while(1) {
        SDL_Delay(5);
        lv_tick_inc(5);         // 现在编译器能找到这个函数了
    }
    return 0;
}

int main(int argc, char **argv) {
    // 1. LVGL 初始化
    lv_init();

    // 2. SDL 初始化
    sdl_init();

    // 3. 注册显示缓冲区
    static lv_color_t buf[800 * 480];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 800 * 480);

    // 4. 注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    lv_disp_drv_register(&disp_drv);

    // 5. 注册输入驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);

    // 6. 创建心跳线程
    SDL_CreateThread(tick_thread, "tick", NULL);

    // 7. 初始化 UI
    ui_init();

    printf("WSL 窗口正在启动...\n");

    while (1) {
        lv_timer_handler(); 
        usleep(5000);
    }
    return 0;
}