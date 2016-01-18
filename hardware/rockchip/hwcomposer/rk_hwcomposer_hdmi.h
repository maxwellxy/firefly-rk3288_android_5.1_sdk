/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/

#define MODE_CHANGE_HDMI 1
#define MODE_ADD_HDMI 2
#define MODE_REMOVE_HDMI 3

void      rk_check_hdmi_uevents(const char *buf);
void      rk_handle_uevents(const char *buff);
void     *rk_hwc_hdmi_thread(void *arg);
extern  int         g_hdmi_mode;
extern  int         mUsedVopNum;
extern  bool        hdmi_noready;
void     hwc_change_config();
void     handle_hotplug_event(int hdmi_mode,int flag);

