#include <stdio.h>
int main(){
    int a, b, c, d, e, f, t1;

    a = 1;
    b = 2;
    c = 6;
    d = 7;
    e = 8;
    f = 9;

    while(a != 100){
        if(f % 2){
            c += 14;
            if(!c % 2){
                t1 = b + 1;
                b = c;
                if(t1 < b){
                    e = 11;
                    if(e == 11){
                        d = 18;
                    }
                }
            }
            f = c;
        }
        a++;
    }

    a = e;
    b = d;

    printf("a:%d b:%d c:%d d:%d e:%d f:%d t1:%d\n",
            a, b, c, d, e, f, t1);

    return 0;
}
