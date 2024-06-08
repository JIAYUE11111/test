#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
static int current_game = 0;

PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

_RegSet* schedule(_RegSet *prev) {
  if(current!=NULL)
    current->tf = prev;
  else current = &pcb[0];
  // current = current == &pcb[0]? &pcb[1] : &pcb[0];
  // current = &pcb[0];
  //pal:hello = 1000:1
  static int pal_freq = 0;
  if(current == &pcb[current_game]){
    pal_freq++;
    // Log("HERE");
    if(pal_freq == 1000){
      current = &pcb[1];
      // Log("switch to hello!");
      pal_freq=0;
    }
  }
  else{
    //always switch hello -> pal
    current->tf = prev;
    current = &pcb[current_game];
    // Log("Switched to pal!");
  }
  //set cr3
  _switch(&current->as);
  return current->tf;
}

void switch_game(){
  if(current_game==0){
    Log("Switch to videotest!");
    current_game = 2;
  }
  else{
    Log("Switch to PAL!");
    current_game = 0;
  }
}