#include <stdio.h>
#include "lv_conf.h"
#include <sdl/sdl.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>
//#include "ui.h"

#define  DISP_BUF_SIZE  800*480*2
void lvgl_init_framebuffer_ts();
#define USD_SDL 1


void grid_layout()
{

    static lv_coord_t col_dsc[] = {LV_PCT(100),LV_PCT(100),LV_PCT(100), LV_PCT(100), LV_PCT(100), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_PCT(100),LV_PCT(100),LV_PCT(100), LV_PCT(100), LV_PCT(100), LV_GRID_TEMPLATE_LAST};

    //创建一个容器,并且放在屏幕上
    lv_obj_t* cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    //设置布局方式为网格布局
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_size(cont, LV_PCT(100),LV_PCT(100));

    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            lv_obj_t *bt = lv_btn_create(cont);
            lv_obj_set_grid_cell(bt, LV_GRID_ALIGN_STRETCH, j, 1,
                                  LV_GRID_ALIGN_STRETCH, i, 1);
        }
    }
}


int main(void)
{
    //初始化（lvgl初始化， 输出初始化， 输入初始化）
    lvgl_init_framebuffer_ts();

    grid_layout();
    //ui_init();

    while(1) { //界面刷新
        lv_timer_handler();
        usleep(5000);
    }
    return  0;
}

void lvgl_init_framebuffer_ts()
{
    /*LittlevGL init*/
    lv_init(); //初始化lvgl
#if USD_SDL
    sdl_init();
#else
    //配置显示接口
    fbdev_init();
    //初始化输入接口
    evdev_init();
#endif
    //输入接口
    static lv_color_t buf[DISP_BUF_SIZE]; //显示缓冲区
    static lv_color_t buf1[DISP_BUF_SIZE]; //显示缓冲区
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, buf1, DISP_BUF_SIZE);

    //设置显示缓冲区属性
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
#if USE_SDL
    disp_drv.flush_cb = sdl_display_flush;
#else
    disp_drv.flush_cb   = fbdev_flush;
#endif
    disp_drv.hor_res    = 800;
    disp_drv.ver_res    = 480;
    lv_disp_drv_register(&disp_drv);


    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;  //设置输入事件类型
    //设置具体采集输入数据接口
#if USE_SDL
    indev_drv_1.read_cb = sdl_mouse_read;
#else
    indev_drv_1.read_cb = evdev_read;
#endif
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

}
/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}