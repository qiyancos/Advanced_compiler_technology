#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long a;
long m;
long q;
long r;
long count;
long res;

void func_0(long n){
    if((n == 0)){
        res = 1;
    }
    else {
        func_0(n - 1);
        res = (n * res);
    }
}

void func_1(long n){
    long y;
    long x;
    if((n <= 1)){
        res = 1;
    }
    else {
        func_1(n - 1);
        x = res;
        func_1(n - 2);
        y = res;
        res = (x + y);
    }
}

void func_2(long from, long to){
    WriteLong(from);
    WriteLong(to);
    WriteLine();
    count = (count + 1);
}

void func_3(long from, long by, long to, long height){
    if((height == 1)){
        func_2(from, to);
    }
    else {
        func_3(from, to, by, height - 1);
        func_2(from, to);
        func_3(by, from, to, height - 1);
    }
}

void func_4(long height){
    count = 0;
    func_3(1, 2, 3, height);
    WriteLine();
    WriteLong(count);
    WriteLine();
}

void main(){
    a = 16807;
    m = 127;
    m = ((m * 256) + 255);
    m = ((m * 256) + 255);
    m = ((m * 256) + 255);
    q = (m / a);
    r = (m % a);
    func_0(7);
    WriteLong(res);
    WriteLine();
    WriteLine();
    func_1(11);
    WriteLong(res);
    WriteLine();
    WriteLine();
    func_4(3);
    WriteLine();
}

