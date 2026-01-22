#include <stdio.h>
#include "lv_conf.h"
#include <sdl/sdl.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>

lv_obj_t * sw1,*sw2, *sw3, *sw4;

void event_handler(lv_event_t *e)
{
    
    if(lv_event_get_target(e) == sw1)
    {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED)
            printf("event-sw1\n");
    }

}
void switch_test()
{
    lv_obj_set_flex_flow(lv_scr_act(), LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lv_scr_act(), LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    sw1 = lv_switch_create(lv_scr_act());
    lv_obj_add_event_cb(sw1, event_handler, LV_EVENT_ALL, NULL);

    sw2 = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw2, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw2, event_handler, LV_EVENT_ALL, NULL);

    sw3 = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw3, LV_STATE_DISABLED);
    lv_obj_add_event_cb(sw3, event_handler, LV_EVENT_ALL, NULL);

    sw4 = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw4, LV_STATE_CHECKED );
    lv_obj_add_event_cb(sw4, event_handler, LV_EVENT_ALL, NULL);

}