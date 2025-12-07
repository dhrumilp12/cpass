#include <stdio.h>
int main(){
    int i, j, k, l;


    i = 0;
    j = 1;
    k = 2;
    l = 3;

    j  = i + 1;
    k  = l;
    k += l;

    i  = j;
    j  = k;
    j += l;

    printf("j:%d i:%d k:%d l:%d\n", j, i, k, l);
    return 0;
}
