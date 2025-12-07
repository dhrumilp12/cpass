// input12.c
#include <stdio.h>

void dump(int a, int b) {
  // No stores, just a read-only call.
  printf("%d %d\n", a, b);
}

int main() {
  int x, y, z;

  x = 3;
  y = 4;

  dump(x, y);      // read-only call

  z = x + y;       // loads of x/y after a pure call

  printf("%d\n", z);
  return 0;
}
