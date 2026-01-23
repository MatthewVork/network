#include <stdio.h>
#include "lv_conf.h"
#include <sdl/sdl.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>
static lv_obj_t* kb = NULL;
static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);  //获取事件
    lv_obj_t * ta = lv_event_get_target(e);  //获取焦点组件
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        /*Focus on the clicked text area*/
        if(kb != NULL) lv_keyboard_set_textarea(kb, ta);
    }
    else if(code == LV_EVENT_READY) {
        LV_LOG_USER("Ready, current text: %s", lv_textarea_get_text(ta));
    }
}
void firsttest(){
    //设计一个登录界面
    lv_obj_t *mainwin = lv_obj_create(lv_scr_act());
    lv_obj_set_align(mainwin,LV_ALIGN_TOP_MID);
    lv_obj_set_size(mainwin,440,200);

    //在mainwin上创建一个输入框
    lv_obj_t* userEdit = lv_textarea_create(mainwin);
    lv_obj_align(userEdit,LV_ALIGN_CENTER,0,-25);
    lv_obj_set_size(userEdit,200,40);
    lv_obj_set_style_bg_color(userEdit,lv_color_hex(0x123456),LV_STYLE_BG_COLOR);
    lv_textarea_set_one_line(userEdit, true);
    lv_textarea_set_placeholder_text(userEdit,"username");


    lv_obj_t *passEdit = lv_textarea_create(mainwin);
    lv_obj_align(passEdit,LV_ALIGN_CENTER,0,25);
    lv_obj_set_size(passEdit,200,40);
    lv_textarea_set_one_line(passEdit, true);
    lv_textarea_set_placeholder_text(passEdit,"password");
    lv_textarea_set_password_mode(passEdit,true);
    
    kb = lv_keyboard_create(lv_scr_act());//键盘
    lv_obj_set_size(kb,  LV_HOR_RES, LV_VER_RES / 2);

    //添加事件
    lv_obj_add_event_cb(userEdit, ta_event_cb, LV_EVENT_ALL,NULL);
    lv_obj_add_event_cb(passEdit, ta_event_cb, LV_EVENT_ALL, NULL);
}