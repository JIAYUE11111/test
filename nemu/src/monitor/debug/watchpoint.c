#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include<stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
  if(!free_){
    printf("Number of watchpoint exceeded! (max: 32)\n");
    assert(0);
  }
  WP* newWP = free_;
  free_ = free_->next;
  newWP->next = head;
  head = newWP;
  return head;
}

void free_wp(WP* wp){
  if(wp->expr){
    free(wp->expr);
  }
  if(head == wp){
    head = wp->next;
  }
  else{
    WP* i;
    for(i = head;i!=NULL;i = i->next){
      if(i->next == wp){
        i->next = wp->next;
      }
    }
  }

  wp->next = free_;
  free_ = wp;
}

WP* getByNo(int idx) {
  WP* i;
  for(i = head;i!=NULL;i = i->next){
    if(i->NO == idx){
      return i;
    }
  }
  return NULL;
}

bool checkWP(){
  bool ret = false;
  WP* i;
  for(i = head;i!=NULL;i = i->next){
    bool success = true;
    uint32_t ans = expr(i->expr, &success);
    if(success){
      if(ans != i->value){
        printf("Watchpoint NO: %d\texpr: %s\t%u->%u\n", i->NO, i->expr, i->value, ans);
        i->value = ans;
        ret = true;
      }
    }
    else{
      assert(0);  //should not reach here
    }
  }
  return ret;
}

void showWP(){
  WP* i;
  for(i = head;i!=NULL;i = i->next){
    printf("Hardware watchpoint %d: %s\tRecent value: %u\n", i->NO, i->expr, i->value);
  }
}