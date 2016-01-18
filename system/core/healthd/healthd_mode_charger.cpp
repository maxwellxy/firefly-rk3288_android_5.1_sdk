/*
 * Copyright (C) 2011-2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <linux/netlink.h>

#include <batteryservice/BatteryService.h>
#include <cutils/android_reboot.h>
#include <cutils/klog.h>
#include <cutils/misc.h>
#include <cutils/uevent.h>
#include <cutils/properties.h>

#ifdef CHARGER_ENABLE_SUSPEND
#include <suspend/autosuspend.h>
#endif

#include "minui/minui.h"

#include "healthd.h"

char *locale;

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#define MSEC_PER_SEC            (1000LL)
#define NSEC_PER_MSEC           (1000000LL)

#define BATTERY_UNKNOWN_TIME    (2 * MSEC_PER_SEC)
#define POWER_ON_KEY_TIME       (2 * MSEC_PER_SEC)
#define UNPLUGGED_SHUTDOWN_TIME (10 * MSEC_PER_SEC)

#define BATTERY_FULL_THRESH     95

#define LAST_KMSG_PATH          "/proc/last_kmsg"
#define LAST_KMSG_PSTORE_PATH   "/sys/fs/pstore/console-ramoops"
#define LAST_KMSG_MAX_SZ        (32 * 1024)

#define LOGE(x...) do { KLOG_ERROR("charger", x); } while (0)
#define LOGW(x...) do { KLOG_WARNING("charger", x); } while (0)
#define LOGV(x...) do { KLOG_DEBUG("charger", x); } while (0)

/* if w0 < w1, return a negative number */
#define GET_CENTER_POS(w0, w1)	(((w0) - (w1)) / 2)

#ifndef CHARGER_CAPACITY_TEXT
#define CHARGER_CAPACITY_TEXT /* remove it if not want to display capacity */
#endif

struct key_state {
    bool pending;
    bool down;
    int64_t timestamp;
};

struct frame {
    const char *name;
    int disp_time;
    int min_capacity;
    bool level_only;

    gr_surface surface;
};

struct capacity_text {
   const char *name;
   int text_count;
   int width;
   int height;
   int cheight;

   gr_surface surface;
};

struct animation {
    bool run;

    struct frame *frames;
    int cur_frame;
    int num_frames;

    int cur_cycle;
    int num_cycles;

    /* current capacity being animated */
    int capacity;
};

struct charger {
    bool have_battery_state;
    bool charger_connected;
    int64_t next_screen_transition;
    int64_t next_key_check;
    int64_t next_pwr_check;

    struct key_state keys[KEY_MAX + 1];

    struct animation *batt_anim;
    gr_surface surf_unknown;
};

static struct frame batt_anim_frames[] = {
    {
        .name = "charger/battery_0",
        .disp_time = 750,
        .min_capacity = 0,
        .level_only = false,
        .surface = NULL,
    },
    {
        .name = "charger/battery_1",
        .disp_time = 750,
        .min_capacity = 20,
        .level_only = false,
        .surface = NULL,
    },
    {
        .name = "charger/battery_2",
        .disp_time = 750,
        .min_capacity = 40,
        .level_only = false,
        .surface = NULL,
    },
    {
        .name = "charger/battery_3",
        .disp_time = 750,
        .min_capacity = 60,
        .level_only = false,
        .surface = NULL,
    },
    {
        .name = "charger/battery_4",
        .disp_time = 750,
        .min_capacity = 80,
        .level_only = true,
        .surface = NULL,
    },
    {
        .name = "charger/battery_5",
        .disp_time = 750,
        .min_capacity = BATTERY_FULL_THRESH,
        .level_only = false,
        .surface = NULL,
    },
};

static struct animation battery_animation = {
    .run = false,
    .frames = batt_anim_frames,
    .cur_frame = 0,
    .num_frames = ARRAY_SIZE(batt_anim_frames),
    .cur_cycle = 0,
    .num_cycles = 3,
    .capacity = 0,
};

static struct capacity_text batt_cap_text[] = {
    {
	.name = "charger/capacity_text_top",
	.text_count = 11,
	.width = 18, /* width and height will be update after load resource */
	.height = 352,
	.cheight = 32,
	.surface = NULL,
    },
    {
	.name = "charger/capacity_text_bottom",
	.text_count = 11,
	.width = 18, /* width and height will be update after load resource */
	.height = 352,
	.cheight = 32,
	.surface = NULL,
    },
    {
	.name = "charger/capacity_text_right",
	.text_count = 11,
	.width = 32, /* width and height will be update after load resource */
	.height = 198,
	.cheight = 18,
	.surface = NULL,
    },
    {
	.name = "charger/capacity_text_left",
	.text_count = 11,
	.width = 32, /* width and height will be update after load resource */
	.height = 198,
	.cheight = 18,
	.surface = NULL,
    },
};

static struct charger charger_state;
static struct healthd_config *healthd_config;
static struct android::BatteryProperties *batt_prop;
static int char_width;
static int char_height;
static bool minui_inited;

/* current time in milliseconds */
static int64_t curr_time_ms(void)
{
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

static void clear_screen(void)
{
    gr_color(0, 0, 0, 255);
    gr_clear();
}

#define MAX_KLOG_WRITE_BUF_SZ 256

static void dump_last_kmsg(void)
{
    char *buf;
    char *ptr;
    unsigned sz = 0;
    int len;

    LOGW("\n");
    LOGW("*************** LAST KMSG ***************\n");
    LOGW("\n");
    buf = (char *)load_file(LAST_KMSG_PSTORE_PATH, &sz);

    if (!buf || !sz) {
        buf = (char *)load_file(LAST_KMSG_PATH, &sz);
        if (!buf || !sz) {
            LOGW("last_kmsg not found. Cold reset?\n");
            goto out;
        }
    }

    len = min(sz, LAST_KMSG_MAX_SZ);
    ptr = buf + (sz - len);

    while (len > 0) {
        int cnt = min(len, MAX_KLOG_WRITE_BUF_SZ);
        char yoink;
        char *nl;

        nl = (char *)memrchr(ptr, '\n', cnt - 1);
        if (nl)
            cnt = nl - ptr + 1;

        yoink = ptr[cnt];
        ptr[cnt] = '\0';
        klog_write(6, "<4>%s", ptr);
        ptr[cnt] = yoink;

        len -= cnt;
        ptr += cnt;
    }

    free(buf);

out:
    LOGW("\n");
    LOGW("************* END LAST KMSG *************\n");
    LOGW("\n");
}

#ifdef CHARGER_ENABLE_SUSPEND
static int request_suspend(bool enable)
{
    if (enable)
        return autosuspend_enable();
    else
        return autosuspend_disable();
}
#else
static int request_suspend(bool /*enable*/)
{
    return 0;
}
#endif

static bool outside(int x, int y)
{
    return x < 0 || x >= gr_fb_width() || y < 0 || y >= gr_fb_height();
}

static int draw_text(const char *str, int x, int y)
{
    int str_len_px = gr_measure(str);

    if (x < 0)
        x = (gr_fb_width() - str_len_px) / 2;
    if (y < 0)
        y = (gr_fb_height() - char_height) / 2;
    gr_text(x, y, str, 1);

    return y + char_height;
}

static void android_green(void)
{
    gr_color(0xa4, 0xc6, 0x39, 255);
}

#ifdef CHARGER_CAPACITY_TEXT

#ifdef CHARGER_CAPACITY_TEXT_USE_FONT
#define SPACING 10
static int draw_charger_capacity(struct charger *charger,
				 int x, int y, int w, int h)
{
    int clen;
    int font_w;
    char str[20] = {0};
    struct animation *batt_anim = charger->batt_anim;

    clen = snprintf(str, sizeof(str) - 1, "%d%%", batt_anim->capacity);
    font_w = clen * gr_get_font_cwidth();
    x += GET_CENTER_POS(w, font_w);

    gr_color(255, 255, 255, 255);
    LOGV("drawing capacity %s xpos=%d ypos=%d\n", str, x, y);
    return draw_text(str, x, y) + SPACING;
}
#else	/* default use images */
#define SPACING 20
static int draw_charger_capacity(struct charger *charger,
				 int x, int y, int w, int h)
{
    int c_w;
    int c_h;
    int c_x;
    int c_y;
    int clen;
    int i;
    int index[4] = {0};
    struct animation *batt_anim = charger->batt_anim;
    int capacity = batt_anim->capacity;
    struct capacity_text *cap_text;

    if (capacity == 100) {
	index[0] = 1;
	index[1] = 0;
	index[2] = 0;
	index[3] = 10; /* '%' */
	clen = 4;
    } else if (capacity > 9) {
	index[0] = capacity / 10;
	index[1] = capacity % 10;
	index[2] = 10;
	clen = 3;
    } else {
	index[0] = capacity % 10;
	index[1] = 10;
	clen = 2;
    }

    if (w <= h) {
#ifdef CHARGER_CAPACITY_TEXT_ROTATE_180 /* on the bottom */
	cap_text = &batt_cap_text[1];
	if (!cap_text->surface)
	    return y;

	c_w = cap_text->width;
	c_h = cap_text->cheight;
	c_x = x + GET_CENTER_POS(w, c_w * clen);
	c_y = GET_CENTER_POS(gr_fb_height(), h + c_h + SPACING) + h + SPACING;

	if (outside(c_x, c_y) || outside(c_x + c_w * clen, c_y + c_h)) {
	    LOGE("drawing outside %dx%d+%d+%d\n", c_x, c_y, c_w * clen, c_h);
	    return y;
	}

	for (i = clen - 1; i >= 0; i--) {
	    LOGV("drawing text %d,%dx%d+%d+%d\n", index[i], c_w, c_h, c_x, c_y);
	    gr_blit(cap_text->surface, 0, c_h * index[i], c_w, c_h, c_x, c_y);
	    c_x += c_w;
	}

	return c_y - h - SPACING;
#else /* default on the top */
	cap_text = &batt_cap_text[0];
	if (!cap_text->surface)
	    return y;

	c_w = cap_text->width;
	c_h = cap_text->cheight;
	c_x = x + GET_CENTER_POS(w, c_w * clen);
	c_y = GET_CENTER_POS(gr_fb_height(), h + c_h + SPACING);

	if (outside(c_x, c_y) || outside(c_x + c_w * clen, c_y + c_h)) {
	    LOGE("drawing outside %dx%d+%d+%d\n", c_x, c_y, c_w * clen, c_h);
	    return y;
	}

	for (i = 0; i < clen; i++) {
	    LOGV("drawing text %d,%dx%d+%d+%d\n", index[i], c_w, c_h, c_x, c_y);
	    gr_blit(cap_text->surface, 0, c_h * index[i], c_w, c_h, c_x, c_y);
	    c_x += c_w;
	}

	return c_y + c_h + SPACING;
#endif
    } else {
#ifdef CHARGER_CAPACITY_TEXT_ROTATE_180 /* on the left */
	cap_text = &batt_cap_text[3];
	if (!cap_text->surface)
	    return x;

	c_w = cap_text->width;
	c_h = cap_text->cheight;
	c_x = GET_CENTER_POS(gr_fb_width(), w + c_w + SPACING);
	c_y = y + GET_CENTER_POS(h, c_h * clen);

	if (outside(c_x, c_y) || outside(c_x + c_w, c_y + c_h * clen)) {
	    LOGE("drawing outside %dx%d+%d+%d\n", c_x, c_y, c_w, c_h * clen);
	    return x;
	}

	for (i = clen - 1; i >= 0; i--) {
	    LOGV("drawing text %d,%dx%d+%d+%d\n", index[i], c_w, c_h, c_x, c_y);
	    gr_blit(cap_text->surface, 0, c_h * index[i], c_w, c_h, c_x, c_y);
	    c_y += c_h;
	}

	return c_x + c_w + SPACING;
#else /* the capacity text display on the right of charging icon by default */
	cap_text = &batt_cap_text[2];
	if (!cap_text->surface)
	    return x;

	c_w = cap_text->width;
	c_h = cap_text->cheight;
	c_x = GET_CENTER_POS(gr_fb_width(), w + c_w + SPACING) + w + SPACING;
	c_y = y + GET_CENTER_POS(h, c_h * clen);

	if (outside(c_x, c_y) || outside (c_x + c_w, c_y + c_h * clen)) {
	    LOGE("drawing outside %dx%d+%d+%d\n", c_x, c_y, c_w, c_h * clen);
	    return x;
	}

	for (i = 0; i < clen; i++) {
	    LOGV("drawing text %d,%dx%d+%d+%d\n", index[i], c_w, c_h, c_x, c_y);
	    gr_blit(cap_text->surface, 0, c_h * index[i], c_w, c_h, c_x, c_y);
	    c_y += c_h;
	}

	return c_x - w - SPACING;
#endif
    }
}
#endif

#endif

/* returns the last y-offset of where the surface ends */
static int draw_surface_centered(struct charger *charger, gr_surface surface)
{
    int w;
    int h;
    int x;
    int y;

    w = gr_get_width(surface);
    h = gr_get_height(surface);
    x = (gr_fb_width() - w) / 2 ;
    y = (gr_fb_height() - h) / 2 ;

#ifdef CHARGER_CAPACITY_TEXT
    /* drawing capacity text */
    if (w <= h)
	y = draw_charger_capacity(charger, x, y, w, h);
    else /* capacity text display on the right of battery icon */
	x = draw_charger_capacity(charger, x, y, w, h);
#endif

    LOGV("drawing surface %dx%d+%d+%d\n", w, h, x, y);
    gr_blit(surface, 0, 0, w, h, x, y);
    return y + h;
}

static void draw_unknown(struct charger *charger)
{
    int y;
    if (charger->surf_unknown) {
        draw_surface_centered(charger, charger->surf_unknown);
    } else {
        android_green();
        y = draw_text("Charging!", -1, -1);
        draw_text("?\?/100", -1, y + 25);
    }
}

static void draw_battery(struct charger *charger)
{
    struct animation *batt_anim = charger->batt_anim;
    struct frame *frame = &batt_anim->frames[batt_anim->cur_frame];

    if (batt_anim->num_frames != 0) {
        draw_surface_centered(charger, frame->surface);
        LOGV("drawing frame #%d name=%s min_cap=%d time=%d\n",
             batt_anim->cur_frame, frame->name, frame->min_capacity,
             frame->disp_time);
    }
}

static void redraw_screen(struct charger *charger)
{
    struct animation *batt_anim = charger->batt_anim;

    clear_screen();

    /* try to display *something* */
    if (batt_anim->capacity < 0 || batt_anim->num_frames == 0)
        draw_unknown(charger);
    else
        draw_battery(charger);
    gr_flip();
}

static void kick_animation(struct animation *anim)
{
    anim->run = true;
}

static void reset_animation(struct animation *anim)
{
    anim->cur_cycle = 0;
    anim->cur_frame = 0;
    anim->run = false;
}

static void update_screen_state(struct charger *charger, int64_t now)
{
    struct animation *batt_anim = charger->batt_anim;
    int cur_frame;
    int disp_time;

    if (!batt_anim->run || now < charger->next_screen_transition)
        return;

    if (!minui_inited) {

        if (healthd_config && healthd_config->screen_on) {
            if (!healthd_config->screen_on(batt_prop)) {
                LOGV("[%" PRId64 "] leave screen off\n", now);
                batt_anim->run = false;
                charger->next_screen_transition = -1;
                if (charger->charger_connected)
                    request_suspend(true);
                return;
            }
        }

        gr_init();
        gr_font_size(&char_width, &char_height);

#ifndef CHARGER_DISABLE_INIT_BLANK
        gr_fb_blank(true);
#endif
        minui_inited = true;
    }

    /* animation is over, blank screen and leave */
    if (batt_anim->cur_cycle == batt_anim->num_cycles) {
        reset_animation(batt_anim);
        charger->next_screen_transition = -1;
        gr_fb_blank(true);
        LOGV("[%" PRId64 "] animation done\n", now);
        if (charger->charger_connected)
            request_suspend(true);
        return;
    }

    disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time;

    /* animation starting, set up the animation */
    if (batt_anim->cur_frame == 0) {
        int ret;

        LOGV("[%" PRId64 "] animation starting\n", now);
        if (batt_prop && batt_prop->batteryLevel >= 0 && batt_anim->num_frames != 0) {
            int i;

            /* find first frame given current capacity */
            for (i = 1; i < batt_anim->num_frames; i++) {
                if (batt_prop->batteryLevel < batt_anim->frames[i].min_capacity)
                    break;
            }
            batt_anim->cur_frame = i - 1;

            /* show the first frame for twice as long */
            disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time * 2;
        }
        if (batt_prop)
            batt_anim->capacity = batt_prop->batteryLevel;
    }

    /* unblank the screen  on first cycle */
    if (batt_anim->cur_cycle == 0)
        gr_fb_blank(false);

    /* draw the new frame (@ cur_frame) */
    redraw_screen(charger);

    /* if we don't have anim frames, we only have one image, so just bump
     * the cycle counter and exit
     */
    if (batt_anim->num_frames == 0 || batt_anim->capacity < 0) {
        LOGV("[%" PRId64 "] animation missing or unknown battery status\n", now);
        charger->next_screen_transition = now + BATTERY_UNKNOWN_TIME;
        batt_anim->cur_cycle++;
        return;
    }

    /* schedule next screen transition */
    charger->next_screen_transition = now + disp_time;

    /* advance frame cntr to the next valid frame only if we are charging
     * if necessary, advance cycle cntr, and reset frame cntr
     */
    if (charger->charger_connected) {
        batt_anim->cur_frame++;

        /* if the frame is used for level-only, that is only show it when it's
         * the current level, skip it during the animation.
         */
        while (batt_anim->cur_frame < batt_anim->num_frames &&
               batt_anim->frames[batt_anim->cur_frame].level_only)
            batt_anim->cur_frame++;
        if (batt_anim->cur_frame >= batt_anim->num_frames) {
            batt_anim->cur_cycle++;
            batt_anim->cur_frame = 0;

            /* don't reset the cycle counter, since we use that as a signal
             * in a test above to check if animation is over
             */
        }
    } else {
        /* Stop animating if we're not charging.
         * If we stop it immediately instead of going through this loop, then
         * the animation would stop somewhere in the middle.
         */
        batt_anim->cur_frame = 0;
        batt_anim->cur_cycle++;
    }
}

static int set_key_callback(int code, int value, void *data)
{
    struct charger *charger = (struct charger *)data;
    int64_t now = curr_time_ms();
    int down = !!value;

    if (code > KEY_MAX)
        return -1;

    /* ignore events that don't modify our state */
    if (charger->keys[code].down == down)
        return 0;

    /* only record the down even timestamp, as the amount
     * of time the key spent not being pressed is not useful */
    if (down)
        charger->keys[code].timestamp = now;
    charger->keys[code].down = down;
    charger->keys[code].pending = true;
    if (down) {
        LOGV("[%" PRId64 "] key[%d] down\n", now, code);
    } else {
        int64_t duration = now - charger->keys[code].timestamp;
        int64_t secs = duration / 1000;
        int64_t msecs = duration - secs * 1000;
        LOGV("[%" PRId64 "] key[%d] up (was down for %" PRId64 ".%" PRId64 "sec)\n",
             now, code, secs, msecs);
    }

    return 0;
}

static void update_input_state(struct charger *charger,
                               struct input_event *ev)
{
    if (ev->type != EV_KEY)
        return;
    set_key_callback(ev->code, ev->value, charger);
}

static void set_next_key_check(struct charger *charger,
                               struct key_state *key,
                               int64_t timeout)
{
    int64_t then = key->timestamp + timeout;

    if (charger->next_key_check == -1 || then < charger->next_key_check)
        charger->next_key_check = then;
}

static void process_key(struct charger *charger, int code, int64_t now)
{
    struct key_state *key = &charger->keys[code];
    int64_t next_key_check;

    if (code == KEY_POWER) {
        if (key->down) {
            int64_t reboot_timeout = key->timestamp + POWER_ON_KEY_TIME;
            if (now >= reboot_timeout) {
                /* We do not currently support booting from charger mode on
                   all devices. Check the property and continue booting or reboot
                   accordingly. */
                if (property_get_bool("ro.enable_boot_charger_mode", false)) {
                    LOGW("[%" PRId64 "] booting from charger mode\n", now);
                    property_set("sys.boot_from_charger_mode", "1");
                } else {
                    LOGW("[%" PRId64 "] rebooting\n", now);
                    android_reboot(ANDROID_RB_RESTART, 0, 0);
                }
            } else {
                /* if the key is pressed but timeout hasn't expired,
                 * make sure we wake up at the right-ish time to check
                 */
                set_next_key_check(charger, key, POWER_ON_KEY_TIME);
            }
        } else {
            /* if the power key got released, force screen state cycle */
            if (key->pending) {
                request_suspend(false);
                kick_animation(charger->batt_anim);
            }
        }
    }

    key->pending = false;
}

static void handle_input_state(struct charger *charger, int64_t now)
{
    process_key(charger, KEY_POWER, now);

    if (charger->next_key_check != -1 && now > charger->next_key_check)
        charger->next_key_check = -1;
}

static void handle_power_supply_state(struct charger *charger, int64_t now)
{
    if (!charger->have_battery_state)
        return;

    if (!charger->charger_connected) {
        request_suspend(false);
        if (charger->next_pwr_check == -1) {
            charger->next_pwr_check = now + UNPLUGGED_SHUTDOWN_TIME;
            LOGW("[%" PRId64 "] device unplugged: shutting down in %" PRId64 " (@ %" PRId64 ")\n",
                 now, (int64_t)UNPLUGGED_SHUTDOWN_TIME, charger->next_pwr_check);
        } else if (now >= charger->next_pwr_check) {
            LOGW("[%" PRId64 "] shutting down\n", now);
            android_reboot(ANDROID_RB_POWEROFF, 0, 0);
        } else {
            /* otherwise we already have a shutdown timer scheduled */
        }
    } else {
        /* online supply present, reset shutdown timer if set */
        if (charger->next_pwr_check != -1) {
            LOGW("[%" PRId64 "] device plugged in: shutdown cancelled\n", now);
            kick_animation(charger->batt_anim);
        }
        charger->next_pwr_check = -1;
    }
}

void healthd_mode_charger_heartbeat()
{
    struct charger *charger = &charger_state;
    int64_t now = curr_time_ms();
    int ret;

    handle_input_state(charger, now);
    handle_power_supply_state(charger, now);

    /* do screen update last in case any of the above want to start
     * screen transitions (animations, etc)
     */
    update_screen_state(charger, now);
}

void healthd_mode_charger_battery_update(
    struct android::BatteryProperties *props)
{
    struct charger *charger = &charger_state;

    charger->charger_connected =
        props->chargerAcOnline || props->chargerUsbOnline ||
        props->chargerWirelessOnline;

    if (!charger->have_battery_state) {
        charger->have_battery_state = true;
        charger->next_screen_transition = curr_time_ms() - 1;
        reset_animation(charger->batt_anim);
        kick_animation(charger->batt_anim);
    }
    batt_prop = props;
}

int healthd_mode_charger_preparetowait(void)
{
    struct charger *charger = &charger_state;
    int64_t now = curr_time_ms();
    int64_t next_event = INT64_MAX;
    int64_t timeout;
    struct input_event ev;
    int ret;

    LOGV("[%" PRId64 "] next screen: %" PRId64 " next key: %" PRId64 " next pwr: %" PRId64 "\n", now,
         charger->next_screen_transition, charger->next_key_check,
         charger->next_pwr_check);

    if (charger->next_screen_transition != -1)
        next_event = charger->next_screen_transition;
    if (charger->next_key_check != -1 && charger->next_key_check < next_event)
        next_event = charger->next_key_check;
    if (charger->next_pwr_check != -1 && charger->next_pwr_check < next_event)
        next_event = charger->next_pwr_check;

    if (next_event != -1 && next_event != INT64_MAX)
        timeout = max(0, next_event - now);
    else
        timeout = -1;

   return (int)timeout;
}

static int input_callback(int fd, unsigned int epevents, void *data)
{
    struct charger *charger = (struct charger *)data;
    struct input_event ev;
    int ret;

    ret = ev_get_input(fd, epevents, &ev);
    if (ret)
        return -1;
    update_input_state(charger, &ev);
    return 0;
}

static void charger_event_handler(uint32_t /*epevents*/)
{
    int ret;

    ret = ev_wait(-1);
    if (!ret)
        ev_dispatch();
}

void healthd_mode_charger_init(struct healthd_config* config)
{
    int ret;
    struct charger *charger = &charger_state;
    int i;
    int epollfd;

    dump_last_kmsg();

    LOGW("--------------- STARTING CHARGER MODE ---------------\n");

    ret = ev_init(input_callback, charger);
    if (!ret) {
        epollfd = ev_get_epollfd();
        healthd_register_event(epollfd, charger_event_handler);
    }

    ret = res_create_display_surface("charger/battery_fail", &charger->surf_unknown);
    if (ret < 0) {
        LOGE("Cannot load battery_fail image\n");
        charger->surf_unknown = NULL;
    }

    charger->batt_anim = &battery_animation;

    for (i = 0; i < charger->batt_anim->num_frames; i++) {
        struct frame *frame = &charger->batt_anim->frames[i];

        ret = res_create_display_surface(frame->name, &frame->surface);
        if (ret < 0) {
            LOGE("Cannot load image %s\n", frame->name);
            /* TODO: free the already allocated surfaces... */
            charger->batt_anim->num_frames = 0;
            charger->batt_anim->num_cycles = 1;
            break;
        }
    }

#ifdef CHARGER_CAPACITY_TEXT
    for (i = 0; i < 4; i++) {
	struct capacity_text *cap_text = &batt_cap_text[i];

	ret = res_create_display_surface(cap_text->name, &cap_text->surface);
	if (ret < 0) {
	    LOGE("Cannot load image %s\n", cap_text->name);
	    cap_text->surface = NULL;
	    break;
	}
	cap_text->width = gr_get_width(cap_text->surface);
	cap_text->height = gr_get_height(cap_text->surface);
	if (cap_text->text_count > 0)
	    cap_text->cheight = cap_text->height / cap_text->text_count;
    }
#endif

    ev_sync_key_state(set_key_callback, charger);

    charger->next_screen_transition = -1;
    charger->next_key_check = -1;
    charger->next_pwr_check = -1;
    healthd_config = config;
}
