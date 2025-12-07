// input11.c
#include <stdio.h>

int g;

void setG(int v) {
  g = v;
}

int main() {
  int x;

  g = 5;          // store1
  setG(10);       // call may change g (and does)
  x = g;          // MUST see 10, not 5

  printf("%d %d\n", g, x);
  return 0;
}
