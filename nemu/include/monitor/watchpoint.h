#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"
//观察点
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char *expr;
  uint32_t value;
} WP;

void showWP();
WP* new_wp();
void free_wp(WP* wp);
WP* getByNo(int i);
bool checkWP();

#endif
