#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long


void main(){
    long array[10];
    long temp;
    long j;
    long i;
    i = 0;
    while((i < 10)){
        array[( i * 8 )>>3] = ((10 - i) - 1);
        i = (i + 1);
    }
    i = 0;
    while((i < 10)){
        WriteLong((array[( i * 8 )>>3]));
        i = (i + 1);
    }
    WriteLine();
    i = 0;
    while((i < 10)){
        j = 0;
        while((j < i)){
            if(!(((array[( j * 8 )>>3]) <= (array[( i * 8 )>>3])))){
                temp = (array[( i * 8 )>>3]);
                array[( i * 8 )>>3] = (array[( j * 8 )>>3]);
                array[( j * 8 )>>3] = temp;
            }
            j = (j + 1);
        }
        i = (i + 1);
    }
    i = 0;
    while((i < 10)){
        WriteLong((array[( i * 8 )>>3]));
        i = (i + 1);
    }
    WriteLine();
}

