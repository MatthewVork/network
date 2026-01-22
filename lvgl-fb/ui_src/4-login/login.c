#include "login.h"

void login_ui()
{
    //获取屏幕
    lv_obj_t* screen = lv_scr_act();
    //创建一个登录框
    lv_obj_t* loginMain = lv_obj_create(screen);
    lv_obj_set_align(loginMain, LV_ALIGN_CENTER);//位置
    lv_obj_set_size(loginMain, 300, 200);//设置大小
    lv_obj_set_flex_flow(loginMain, LV_FLEX_FLOW_ROW_WRAP);
    //在loginMain窗口上创建输入框
    lv_obj_t* username = lv_textarea_create(loginMain);
    lv_obj_set_size(username, LV_PCT(50), LV_SIZE_CONTENT);//设置大小
    //lv_obj_align(username, LV_ALIGN_TOP_MID, 0,30);
    lv_textarea_set_placeholder_text(username,"username");
    lv_textarea_set_one_line(username,true);
    
    //lv_textarea_set_align(username,LV_TEXT_ALIGN_CENTER);

    lv_obj_t* password = lv_textarea_create(loginMain);
    lv_obj_set_size(password, LV_PCT(50), LV_SIZE_CONTENT);//设置大小
    //lv_obj_align(password, LV_ALIGN_TOP_MID, 0,100);
    lv_textarea_set_placeholder_text(password,"password");
    lv_textarea_set_password_mode(password, true);
    lv_textarea_set_one_line(password,true);

    //创建按钮
    lv_obj_t* loginBt = lv_btn_create(loginMain);
    lv_obj_set_size(loginBt, LV_PCT(100), LV_SIZE_CONTENT);//设置大小
    //lv_obj_align(loginBt, LV_ALIGN_BOTTOM_MID, 0,-10);
    //按钮文本
    lv_obj_t* buttontext = lv_label_create(loginBt);
    lv_label_set_text(buttontext, "login");
    lv_obj_center(buttontext);
    //lv_obj_set_align(buttontext,LV_ALIGN_CENTER);
}