#include <stdio.h>
int main(){
    int a, b, c, d, e, f;
    int t1, t2;

    a = 1;
    b = 2;
    c = 6;
    d = 7;
    e = 10;
    f = 11;

    for(a = 0; a <= 10; a++){
        t1 = a + 19;
        b = 8;
    }

    c  = a;
    f  = b;
    t2 = e;

    printf("a:%d b:%d c:%d d:%d e:%d f:%d t1:%d t2:%d\n",
            a, b, c, d, e, f, t1, t2);

    return 0;
}
