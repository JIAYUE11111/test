#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NEQ, TK_OR, TK_AND, TK_NOT, TK_NGTIVE, TK_DEREF, TK_INT, TK_HEX, TK_REG
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ}, //not equal
  {"&&", TK_AND}, 
  {"\\|\\|", TK_OR}, 
  {"!", TK_NOT}, 
  {"0x[0-9a-fA-F][0-9a-fA-F]*", TK_HEX}, //hex
  {"[1-9][0-9]*|0", TK_INT}, //integer
  {"\\$[a-zA-z]+", TK_REG}, //register
  {"-", '-'},  //sub
  {"\\*", '*'}, //mul
  {"/", '/'}, //div
  {"\\(", '('}, //lbranket
  {"\\)", ')'}  //rbranket
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          default:
            if(nr_token >= 32){
              printf("expression too long!");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            // printf("%d\t%s\n", tokens[nr_token].type, tokens[nr_token].str);
            nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q){
  int count = 0;
  int i;
  for(i = p;i<q;i++){
    if(tokens[i].type=='('){
      count++;
    }
    else if(tokens[i].type==')'){
      count--;
    }
    if(count<=0){
      if(count<0){  //brankets unmatched
        printf("( and ) unmatched!\n");
        assert(0);
      }
      else return false;  //not in the (<expr>) format
    }
  }
  if(count != 1 || tokens[q].type!=')'){
    printf("( and ) unmatched!\n");
    assert(0);
  }
  return true;
}

uint32_t eval(int p,int q) {
  if (p>q) {
    /* Bad expression */
    printf("Bad expression\n");
    assert(0);
  }
  else if (p == q){
    /* Single token.
     * For now this token should be a number.
     *Return the value of the number.
     */
    uint32_t ret = 0;
    switch(tokens[p].type){
      case TK_INT:
        sscanf(tokens[p].str, "%d", &ret);
        return ret;
      case TK_HEX:
        sscanf(tokens[p].str, "%x", &ret);
        return ret;
      case TK_REG:
        if(strcasecmp(tokens[p].str, "$eax")==0){
          return cpu.eax;
        }
        else if(strcasecmp(tokens[p].str, "$ecx")==0){
          return cpu.ecx;
        }
        else if(strcasecmp(tokens[p].str, "$edx")==0){
          return cpu.edx;
        }
        else if(strcasecmp(tokens[p].str, "$ebx")==0){
          return cpu.ebx;
        }
        else if(strcasecmp(tokens[p].str, "$esp")==0){
          return cpu.esp;
        }
        else if(strcasecmp(tokens[p].str, "$ebp")==0){
          return cpu.ebp;
        }
        else if(strcasecmp(tokens[p].str, "$esi")==0){
          return cpu.esi;
        }
        else if(strcasecmp(tokens[p].str, "$edi")==0){
          return cpu.edi;
        }
        else if(strcasecmp(tokens[p].str, "$eip")==0){
          return cpu.eip;
        }
      default:
        printf("Invalid operand!\n");
        assert(0);
    }
  }
  else if (check_parentheses(p,q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1,q - 1);
  }
  else {
    int singleLevel = -1; //* - !
    int mulLevel = -1;  //* /
    int addLevel = -1;  //+ -
    int andLevel = -1;  //&&
    int orLevel = -1;  //||
    int equalLevel = -1;  //== !=
    int numBranket = 0;
    int i;
    for(i = p;i<=q;i++){
      if(tokens[i].type=='('){
        numBranket++;
        continue;
      }
      else if(tokens[i].type==')'){
        numBranket--;
        if(numBranket<0){
          printf("( and ) unmatched!\n");
          assert(0);
        }
        continue;
      }
      else if(!numBranket){//not in brankets
        switch (tokens[i].type)
        {
        case TK_DEREF: case TK_NGTIVE: case TK_NOT:
          if(singleLevel<0){
            singleLevel = i;
          }
          break;
        case '*': case '/':
          mulLevel = i;
          break;
        case '+': case '-':
          addLevel = i;
          break;
        case TK_AND:
          andLevel = i;
          break;
        case TK_OR:
          orLevel = i;
          break;
        case TK_EQ: case TK_NEQ:
          equalLevel = i;
          break;
        }
      }
    }
    int op = singleLevel;
    if(equalLevel>=0){
      op = equalLevel;
    }
    else if(orLevel>=0){
      op = orLevel;
    }
    else if(andLevel>=0){
      op = andLevel;
    }
    else if(addLevel>=0){
      op = addLevel;
    }
    else if(mulLevel>=0){
      op = mulLevel;
    }
    assert(op>=0);
    uint32_t val2 = eval(op + 1, q);
    if(op==p){  //* - !
      switch (tokens[op].type)
      {
      case TK_DEREF:
        return vaddr_read(val2, 4);
      case TK_NGTIVE:
        return -val2;
      case TK_NOT:
        return !val2;
      default:
        printf("Invalid single opcode!\n");
        assert(0);
      }
    }
    uint32_t val1 = eval(p, op - 1);
    // printf("%d\t%d\n", val1, val2);
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;
      default: assert(0);
    }
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  int i;
  for (i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && 
      (i == 0 || (tokens[i - 1].type != TK_INT && 
      tokens[i - 1].type != TK_HEX && 
      tokens[i - 1].type != TK_REG &&
      tokens[i - 1].type != ')'))){
        tokens[i].type = TK_DEREF;
      }
    else if (tokens[i].type == '-' && 
      (i == 0 || (tokens[i - 1].type != TK_INT && 
      tokens[i - 1].type != TK_HEX && 
      tokens[i - 1].type != TK_REG &&
      tokens[i - 1].type != ')'))){
        tokens[i].type = TK_NGTIVE;
      }
  }
  return eval(0, nr_token - 1);
}
