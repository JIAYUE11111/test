#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int count;

  if (arg == NULL) {
    /* no argument given */
    count = 1;
  }
  else {
    sscanf(arg, "%d", &count);
  }
  cpu_exec(count);
  return 0;
}

static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (strcmp(arg, "r") == 0) {  //print register
    printf("eax\t0x%x\t%d\n", cpu.eax, cpu.eax);
    printf("ebx\t0x%x\t%d\n", cpu.ebx, cpu.ebx);
    printf("ecx\t0x%x\t%d\n", cpu.ecx, cpu.ecx);
    printf("edx\t0x%x\t%d\n", cpu.edx, cpu.edx);
    printf("esi\t0x%x\t%d\n", cpu.esi, cpu.esi);
    printf("edi\t0x%x\t%d\n", cpu.edi, cpu.edi);
    printf("ebp\t0x%x\t%d\n", cpu.ebp, cpu.ebp);
    printf("esp\t0x%x\t%d\n", cpu.esp, cpu.esp);
    printf("eip\t0x%x\t0x%x\n", cpu.eip, cpu.eip);
  }
  else if(strcmp(arg, "w") == 0){
    showWP();
  }
  else{
    printf("Invalid arguments\n");
  }
  return 0;
}

static int cmd_x(char *args) {
  /* extract the first argument */
  char *arg1 = strtok(NULL, " ");
  char *arg2 = strtok(NULL, " ");

  if (arg1 == NULL || arg2 == NULL) {
    /* no argument given */
    printf("Invalid arguments\n");
    return 0;
  }
  else {
    int count, addr;
    sscanf(arg1, "%d", &count);
    sscanf(arg2, "%x", &addr);
    for(;count>0;count--){
      printf("0x%x\t0x%x\n", addr, vaddr_read(addr, 4));
      addr += 4;
    }
  }
  return 0;
}

static int cmd_p(char *args) {
  char *arg = strtok(NULL, "\n");
  bool success = true;
  uint32_t ans = expr(arg, &success);
  if(success){
    printf("%u\n", ans);
  }
  else{
    printf("Cannot recognize the pattern!\n");
  }
  return 0;
}

static int cmd_w(char *args) {
  WP* wp = new_wp();
  char* arg = strtok(NULL, "\n");
  wp->expr = malloc(strlen(arg)+1);
  strcpy(wp->expr, arg);
  bool success = true;
  wp->value = expr(arg, &success);
  if(success){
    printf("Hardware watchpoint %d: %s\tRecent value: %u\n", wp->NO, wp->expr, wp->value);
  }
  else{
    printf("Cannot recognize the pattern! Insertion failed!\n");
    free_wp(wp);
  }
  return 0;
}

static int cmd_d(char *args) {
  char* arg = strtok(NULL, "\n");
  int num = -1;
  sscanf(arg, "%d", &num);
  WP* point = getByNo(num);
  if(point){
    free_wp(point);
  }
  else{
    printf("No watchpoint with id %d!\n", num);
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Execute and pause after N commands, N is set to 1 as default", cmd_si },
  { "info", "Display register status (r) or information of the watching points (w)", cmd_info },
  { "p", "Caluculate the value of the expression", cmd_p },
  { "x", "Memory scanning", cmd_x },
  { "w", "Set a watching point", cmd_w },
  { "d", "Delete a watching point", cmd_d },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
