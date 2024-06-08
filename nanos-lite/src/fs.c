#include "fs.h"

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern void get_screen(int *width, int *height);
extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern void fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  int width, height;
  get_screen(&width, &height);
  file_table[FD_FB].size = width * height * 4;
}

size_t fs_filesz(int fd){
  return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode){
  int i;
  for(i = 0;i<NR_FILES;i++){
    if(strcmp(pathname, file_table[i].name)==0){
      return i;
    }
  }
  // assert(0);
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len){
  int maxlen = fs_filesz(fd) - file_table[fd].open_offset;
  if(maxlen < len && fd != FD_EVENTS){
    len = maxlen;
  }
  switch (fd)
  {
  case FD_STDOUT:
  case FD_STDERR:
  case FD_FB:
    assert(0);
    break;
  case FD_EVENTS:
    return events_read(buf, len);
  case FD_DISPINFO:
    dispinfo_read(buf, file_table[fd].open_offset, len);
    file_table[fd].open_offset += len;
    break;
  default:
    ramdisk_read(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
  }
  return len;
}

ssize_t fs_write(int fd, const void *buf, size_t len){
  switch (fd)
  {
  case FD_STDOUT:
  case FD_STDERR:
  {
    int i;
    for(i = 0;i<len;i++){
      _putc(((char*)buf)[i]);
    }
    return len;
  }
  case FD_FB:
    fb_write(buf, file_table[fd].open_offset, len);
    file_table[fd].open_offset += len;
    return len;
  case FD_DISPINFO:
    assert(0);
  default:
  {
    int maxlen = fs_filesz(fd) - file_table[fd].open_offset;
    if(maxlen < len){
     len = maxlen;
    }
    ramdisk_write(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
  }
  }
  return len;
}

off_t fs_lseek(int fd, off_t offset, int whence){
  switch (whence)
  {
  case SEEK_SET:
    file_table[fd].open_offset = offset;
    break;
  case SEEK_CUR:
    file_table[fd].open_offset += offset;
    break;
  case SEEK_END:
    file_table[fd].open_offset = file_table[fd].size + offset;
    break;
  }
  if(file_table[fd].open_offset > file_table[fd].size){
    file_table[fd].open_offset = file_table[fd].size;
  }
  else if (file_table[fd].open_offset < 0){
    file_table[fd].open_offset = 0;
  }
  return file_table[fd].open_offset;
}

int fs_close(int fd){
  file_table[fd].open_offset = 0;
  return 0;
}