#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long res;

void main(){
    long b[10];
    long a;
    a = 3;
    res = a;
    b[3] = 4;
    b[4] = 1;
    a = (b[4] + b[3]);
    b[5] = 7;
    WriteLong((b[( a * 8 )>>3]));
    b[3] = 67;
    b[4] = (b[3] + a);
    res = 4;
    WriteLong((b[( res * 8 )>>3]));
    if(!(0)){
        res = (res + 1);
    }
    else {
        res = (res - 1);
    }
    b[( res * 8 )>>3] = 89;
    WriteLong((b[( res * 8 )>>3]));
    WriteLong(b[3]);
    WriteLong(a);
}

