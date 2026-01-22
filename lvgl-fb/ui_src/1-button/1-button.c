#include <stdio.h>
#include "lv_conf.h"
#include <sdl/sdl.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>

lv_style_t * default_style() //创建按钮的默认样式
{
    /*Init the style for the default state*/
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_radius(&style, 3);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_color(&style, lv_palette_darken(LV_PALETTE_BLUE, 2));
    lv_style_set_bg_grad_dir(&style, LV_GRAD_DIR_VER);

    lv_style_set_border_opa(&style, LV_OPA_40);
    lv_style_set_border_width(&style, 2);
    lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&style, 8);
    lv_style_set_shadow_color(&style, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_shadow_ofs_y(&style, 8);

    lv_style_set_outline_opa(&style, LV_OPA_COVER);
    lv_style_set_outline_color(&style, lv_palette_main(LV_PALETTE_BLUE));

    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_pad_all(&style, 10);
    return &style;
}
lv_style_t * pressed_style() //创建按钮的按下样式
{
    /*Init the pressed style*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);

    /*Add a large outline when pressed*/
    lv_style_set_outline_width(&style_pr, 30);
    lv_style_set_outline_opa(&style_pr, LV_OPA_TRANSP);

    lv_style_set_translate_y(&style_pr, 5);
    lv_style_set_shadow_ofs_y(&style_pr, 3);
    lv_style_set_bg_color(&style_pr, lv_palette_darken(LV_PALETTE_BLUE, 2));
    lv_style_set_bg_grad_color(&style_pr, lv_palette_darken(LV_PALETTE_BLUE, 4));

    /*Add a transition to the outline*/
    static lv_style_transition_dsc_t trans;
    static lv_style_prop_t props[] = {LV_STYLE_OUTLINE_WIDTH, LV_STYLE_OUTLINE_OPA, 0};
    lv_style_transition_dsc_init(&trans, props, lv_anim_path_linear, 300, 0, NULL);

    lv_style_set_transition(&style_pr, &trans);
    return  &style_pr;
}


 void my_event_cb(lv_event_t * e)
 {
    //lv_event_get_target  --获取事件发生对象
    //lv_event_get_code -- 获取事件类型
    //lv_event_get_user_data --获取注册函数的时候传入参数
    static int i=0;
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        printf("event:%d\n", i++);
    }
 }

void button_test()
{
    lv_obj_t *screen = lv_scr_act(); //获取屏幕
    lv_obj_t *button = lv_btn_create(screen);//在屏幕上创建按钮
    //设置按钮大小
    lv_obj_set_size(button, 200, 60); //宽200， 高60
    //设置按钮位置
    //lv_obj_set_align(button, LV_ALIGN_BOTTOM_RIGHT);
    //lv_obj_align(button, LV_ALIGN_TOP_LEFT, 200,0);
    lv_obj_set_pos(button, 100, 100); //设置x=100,y=100
    //设置按钮文本--按钮上的文本其实是一个标签
    lv_obj_t *buttontext = lv_label_create(button);//在button上创建一个标签
    //设置标签的文本
    lv_label_set_text(buttontext,"start");
    //设置标签的位置
    lv_obj_align(buttontext, LV_ALIGN_CENTER,0,0);
    //设置按钮颜色
    lv_style_t *style = default_style();
    lv_style_t *style_pr = pressed_style();
    //lv_obj_remove_style_all(button);  //把原来的样式删除                        /*Remove the style coming from the theme*/
    lv_obj_add_style(button, style, 0);//添加默认样式
    lv_obj_add_style(button, style_pr, LV_STATE_PRESSED); //添加按下样式
    //设置按钮点击做的事情(给对象添加事件)
    lv_obj_add_event_cb(button, my_event_cb, LV_EVENT_ALL, NULL);

}