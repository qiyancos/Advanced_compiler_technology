#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long

long prime[400];

void main(){
    long v;
    long j;
    long i;
    prime[0] = 2;
    WriteLong(prime[0]);
    i = 1;
    v = 3;
    while((i < 400)){
        j = 0;
        while((j < i)){
            if(!((((prime[( j * 8 )>>3]) * (prime[( j * 8 )>>3])) <= v))){
                j = (i - 1);
            }
            else {
                if(((v % (prime[( j * 8 )>>3])) == 0)){
                    j = i;
                }
            }
            j = (j + 1);
        }
        if((j == i)){
            prime[( i * 8 )>>3] = v;
            WriteLong(v);
            i = (i + 1);
        }
        v = (v + 2);
    }
    WriteLine();
}

