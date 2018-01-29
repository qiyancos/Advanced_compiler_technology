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
    res = 3;
    b[3] = 4;
    b[4] = 1;
    a = 5;
    b[5] = 7;
    WriteLong(7);
    b[3] = 67;
    b[4] = 72;
    res = 4;
    WriteLong(72);
    if(!(0)){
        res = 5;
    }
    else {
        res = 3;
    }
    b[( res * 8 )>>3] = 89;
    WriteLong((b[( res * 8 )>>3]));
    WriteLong(b[3]);
    WriteLong(5);
}

