#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long res;

void main(){
    long b[11];
    b[5] = 7;
    WriteLong(7);
    b[3] = 67;
    b[4] = 72;
    WriteLong(72);
    if(!(0)){
        res = 5;
    }
    else {
    }
    b[( res * 8 )>>3] = 89;
    WriteLong((b[( res * 8 )>>3]));
    WriteLong(b[3]);
    WriteLong(5);
}

