#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long a[8];
long b[24];

void main(){
    long c[8];
    long i;
    c[6] = 987654321;
    a[0] = 1;
    a[1] = 2;
    c[0] = 9;
    c[1] = 0;
    b[0] = 3;
    b[1] = 4;
    b[(( a[0] * 64 ))>>3] = 5;
    b[(( a[0] * 64 ) + 8)>>3] = 6;
    b[(( ((b[(( (a[0] - 1) * 64 ))>>3]) - 1) * 64 ))>>3] = 7;
    b[(( ((b[(( (a[1] - 2) * 64 ))>>3]) - 1) * 64 ) + 8)>>3] = 8;
    WriteLong(a[0]);
    WriteLong(a[1]);
    WriteLine();
    i = 0;
    while((i < 3)){
        WriteLong((b[(( i * 64 ))>>3]));
        WriteLong((b[(( i * 64 ) + 8)>>3]));
        WriteLine();
        i = (i + 1);
    }
    WriteLong(c[0]);
    WriteLong(c[1]);
    WriteLine();
    WriteLong(c[6]);
    WriteLine();
}

