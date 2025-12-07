#include <stdio.h>
int main(){
    int a, b, c, d, e, f, g;

    a = 1;
    b = 2;
    c = 6;
    d = 7;
    e = 8;
    f = 9;

    if (a < b) {
        g = a + 1;
        b = 6;
    }

    a = b;
    d = c;

    printf("a:%d b:%d c:%d d:%d e:%d f:%d g:%d", a, b, c, d, e, f, g);
    return 0;
}
