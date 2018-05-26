#include "mylibc.h"

_Device* find_device(uint32_t device_id) {
    for (int i = 1;; i++) {
        _Device* dev = _device(i);
        if (!dev)
            break;
        if (dev->id == device_id)
            return dev;
    }
    return NULL;
}

uint32_t uptime() {
    _UptimeReg uptime;
    _Device* dev = find_device(_DEV_TIMER);
    dev->read(_DEVREG_TIMER_UPTIME, &uptime, sizeof(uptime));
    return uptime.lo;
}

_KbdReg read_key() {
    _KbdReg kbd;
    _Device* dev = find_device(_DEV_INPUT);
    dev->read(_DEVREG_INPUT_KBD, &kbd, sizeof(kbd));
    return kbd;
}

void draw_rect(uint32_t* pixels, int x, int y, int w, int h) {
    _Device* dev = find_device(_DEV_VIDEO);
    _FBCtlReg ctl;
    ctl.x = x;
    ctl.y = y;
    ctl.w = w;
    ctl.h = h;
    ctl.sync = 1;
    ctl.pixels = pixels;
    dev->write(_DEVREG_VIDEO_FBCTL, &ctl, sizeof(ctl));
}

void draw_sync() {
    /* nothing */;
}

int screen_width() {
    _Device* dev = find_device(_DEV_VIDEO);
    _VideoInfoReg info;
    dev->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
    return info.width;
}

int screen_height() {
    _Device* dev = find_device(_DEV_VIDEO);
    _VideoInfoReg info;
    dev->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
    return info.height;
}
