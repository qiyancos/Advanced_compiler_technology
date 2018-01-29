#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

long res;

void main(){
    long a;
    long b[10];
    a = 3;
    res = a;
    b[3] = 4;
    b[4] = 1;
    a = b[4] + b[3];
    b[5] = 7;
    WriteLong(b[a]);
    b[3] = 67;
    b[4] = b[3] + a;
    res = 4;
    WriteLong(b[res]);
    if( 1 > 0 ){
        res = res + 1;
    }
    else {
        res = res - 1;
    }
    b[res] = 89;
    WriteLong(b[res]);
    WriteLong(b[3]);
    WriteLong(a);
}

