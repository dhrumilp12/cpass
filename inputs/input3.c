#include <stdio.h>
int main(){
    int a, b, c, d, e, f;

    a = 1;
    b = 2;
    c = 6;
    d = 7;
    e = 8;
    f = 9;

    if(a < 2){
        f = a + e;
        c = 10;
        if(f > 0){
            f = 0;
        }
        else{
            f = d;
        }
    }

    c = a;
    e = f;

    printf("a:%d b:%d c:%d d:%d e:%d f:%d", a, b, c, d, e, f);

    return 0;
}
