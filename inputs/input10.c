// input10.c
#include <stdio.h>

int main() {
  int i, sum, last;

  sum = 0;
  last = -1;
  i = 0;

  while (i < 5) {
    sum = sum + i;
    last = sum;                   // store each iteration
    printf("%d %d\n", sum, last); // some loads inside the loop
    i = i + 1;
  }

  // Here CPIn at this block should reflect the fixed point across the loop.
  printf("%d %d\n", sum, last);
  return 0;
}
