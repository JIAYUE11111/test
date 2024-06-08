#include "common.h"
#define KEYDOWN_MASK 0x8000

#define NAME(key) \
  [_KEY_##key] = #key,

extern void get_screen(int *width, int *height);

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char temp[100];
  int keycode = _read_key();
  //clock event
  if (keycode == _KEY_NONE){
    sprintf(temp, "t %d\n", _uptime());
    // Log("%s", temp);
  }
  else{
    if((keycode & KEYDOWN_MASK) != 0){
      keycode = keycode & ~KEYDOWN_MASK;
      sprintf(temp, "kd %s\n", keyname[keycode]);
      // Log("%s", temp);
      if(keycode == _KEY_F12){
        Log("F12!");
        extern void switch_game();
        switch_game();
      }
    }
    else{
      sprintf(temp, "ku %s\n", keyname[keycode]);
      // Log("%s", temp);
    }
  }

  if(strlen(temp)<=len){
    strncpy((char*)buf, temp, strlen(temp));
    return strlen(temp);
  }
  // else{
  //   strncpy((char*)buf, temp, len);
  //   return len;
  // }
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
  // Log("%s", buf);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  // one pixel is worth 4 bytes
  int start = offset / 4;
  int end = (offset + len)/4;
  int width, height;
  get_screen(&width, &height);
  int x1 = start % width;
  int y1 = start / width;
  int y2 = end / width;
  // Log("%d\t%d\n", width, height);

  //one line
  if(y1==y2){
    _draw_rect(buf, x1, y1, len/4, 1);
  }
  //two lines
  else if(y2 == y1 + 1){
    _draw_rect(buf, x1, y1, width - x1, 1);
    _draw_rect(buf + (width - x1) * 4, 0, y2, len/4 + x1 - width, 1);
  }
  //more than three lines
  else{
    int first_width = width - x1;
    int last_width = (len/4 - first_width) % width;
    if(last_width == 0){
      last_width = width;
    }
    _draw_rect(buf, x1, y1, first_width, 1);
     _draw_rect(buf + first_width * 4, 0, y1 + 1, width, y2 - y1 - 1);
    _draw_rect(buf + len - last_width * 4, 0, y2, last_width, 1);
  }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  int width, height;
  get_screen(&width, &height);
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", width, height);
  // Log("%s", dispinfo);
}
