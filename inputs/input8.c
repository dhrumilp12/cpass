// input8.c
#include <stdio.h>

int main() {
  int x, y, z;

  x = 1;
  y = 2;

  if (x > 0) {
    y = 5;       // path 1: y = 5
  } else {
    // path 2: y is still 2
  }

  // At the merge, CPIn for y should be empty (2 vs 5 disagree),
  // so no store propagation here.
  z = y;

  printf("%d %d %d\n", x, y, z);
  return 0;
}
