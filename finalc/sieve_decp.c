#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long


void main(){
    long is_prime[100];
    long j;
    long i;
    is_prime[0] = 0;
    is_prime[1] = 0;
    i = 2;
    while((i < 100)){
        is_prime[( i * 8 )>>3] = 1;
        i = (i + 1);
    }
    i = 2;
    while((i < 100)){
        if(!(((is_prime[( i * 8 )>>3]) == 0))){
            j = 2;
            while(((i * j) < 100)){
                is_prime[( (i * j) * 8 )>>3] = 0;
                j = (j + 1);
            }
        }
        i = (i + 1);
    }
    i = 2;
    while((i < 100)){
        if(!(((is_prime[( i * 8 )>>3]) == 0))){
            WriteLong(i);
        }
        i = (i + 1);
    }
    WriteLine();
}

