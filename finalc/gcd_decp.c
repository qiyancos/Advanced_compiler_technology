#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long a;
long b;
long res;

void func_0(long a, long b){
    long c;
    while(!((b == 0))){
        c = a;
        a = b;
        b = (c % b);
        WriteLong(c);
        WriteLong(a);
        WriteLong(b);
        WriteLine();
    }
    res = a;
}

void main(){
    a = 25733;
    b = 48611;
    func_0(a, b);
    WriteLong(res);
    WriteLine();
    WriteLine();
    a = 7485671;
    b = 7480189;
    func_0(a, b);
    WriteLong(res);
    WriteLine();
}

