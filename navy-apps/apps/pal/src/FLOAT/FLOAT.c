#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

typedef struct {
  unsigned int fraction: 23;
  unsigned int exponent: 8;
  unsigned int sign: 1;
}float_transformer;

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return ((int64_t)a*(int64_t)b)>>16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  assert(b != 0);
  int64_t a_long = ((int64_t)a)<<16;
  FLOAT result = a/b;
  return result;
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */
  float_transformer *trans = (float_transformer *)(&a);
  int shift = (int)(trans->exponent) - 134;
  unsigned int bits = trans->fraction | (1<<23);
  if(shift>0){
    bits  = bits << shift;
  }
  else if(shift<0){
    bits  = bits >> (-shift);
  }
  else if(bits == (1<<23)){
    bits=0;
  }
  if(trans->sign & 1){
    bits = -bits;
  }
  return bits;
}

FLOAT Fabs(FLOAT a) {
  return a>0?a:-a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
