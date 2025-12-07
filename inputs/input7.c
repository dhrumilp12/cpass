// input7.c
#include <stdio.h>

int main() {
  int a, b, c;

  a = 1;
  b = 2;
  c = 3;

  a = 10;
  b = a;          // load of a should be propagated
  c = a + b;      // both operands should be known

  printf("%d %d %d\n", a, b, c);
  return 0;
}
