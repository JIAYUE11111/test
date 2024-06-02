#include "common.h"
#include "fs.h"
#include "memory.h"

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))
#define DEFAULT_ENTRY ((void *)0x8048000)

uintptr_t loader(_Protect *as, const char *filename) {
  // ramdisk_read(DEFAULT_ENTRY, 0, RAMDISK_SIZE);
  int fd = fs_open(filename, 0, 0);
  // fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));

  int num_pages = fs_filesz(fd) / PGSIZE;
  if(num_pages%PGSIZE != 0){
    num_pages++;
  }
  void* p = DEFAULT_ENTRY;
  for(int i = 0;i<num_pages;i++){
    void* pa = new_page();
    _map(as, p, pa);
    fs_read(fd, pa, PGSIZE);
    p += PGSIZE;
  }

  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
