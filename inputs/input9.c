// input9.c
#include <stdio.h>

int main() {
  int x, y, z;

  x = 1;

  if (x > 0) {
    y = 5;
  } else {
    y = 5;
  }

  // Both predecessors agree: y = 5.
  // CPIn at the merge should contain the copy for y.
  z = y;

  printf("%d %d %d\n", x, y, z);
  return 0;
}
