#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  unsigned long cur_time = inl(RTC_PORT);
  return cur_time - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

static inline int min(int x, int y) {
  return (x < y) ? x : y;
}

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  int i;
  // for (i = 0; i < _screen.width * _screen.height; i++) {
  //   fb[i] = i;
  // }
  int cp_bytes = sizeof(uint32_t) * min(w, _screen.width - x);
  for (i = 0; i < h && y + i < _screen.height; i ++) {
    memcpy(&fb[(y + i) * _screen.width + x], pixels, cp_bytes);
    pixels += w;
  }
}

void _draw_sync() {
}

int _read_key() {
  uint8_t status = inb(I8042_STATUS_PORT);
  if(status){
    return inl(I8042_DATA_PORT);
  }
  return _KEY_NONE;
}

void get_screen(int *width, int *height){
  *width = _screen.width;
  *height = _screen.height;
}