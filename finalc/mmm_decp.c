#include <stdio.h> 
 #define WriteLine() printf("\n"); 
 #define WriteLong(x) printf(" %lld", (long)x); 
 #define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0; 
 #define long long long


void main(){
    long k;
    long j;
    long i;
    long m3[9];
    long m2[12];
    long m1[12];
    i = 0;
    while((i < 4)){
        j = 0;
        while((j < 3)){
            m1[(( i * 24 ) + (j * 8))>>3] = (i + (j * 2));
            WriteLong((i + (j * 2)));
            j = (j + 1);
        }
        WriteLine();
        i = (i + 1);
    }
    i = 0;
    while((i < 4)){
        j = 0;
        while((j < 3)){
            m2[(( j * 32 ) + (i * 8))>>3] = (m1[(( i * 24 ) + (j * 8))>>3]);
            j = (j + 1);
        }
        i = (i + 1);
    }
    WriteLine();
    i = 0;
    while((i < 3)){
        j = 0;
        while((j < 4)){
            WriteLong((m2[(( i * 32 ) + (j * 8))>>3]));
            j = (j + 1);
        }
        WriteLine();
        i = (i + 1);
    }
    i = 0;
    while((i < 3)){
        j = 0;
        while((j < 3)){
            m3[(( i * 24 ) + (j * 8))>>3] = 0;
            j = (j + 1);
        }
        i = (i + 1);
    }
    WriteLine();
    i = 0;
    while((i < 3)){
        j = 0;
        while((j < 3)){
            k = 0;
            while((k < 4)){
                m3[(( i * 24 ) + (j * 8))>>3] = ((m3[(( i * 24 ) + (j * 8))>>3]) + ((m1[(( k * 24 ) + (j * 8))>>3]) * (m2[(( i * 32 ) + (k * 8))>>3])));
                k = (k + 1);
            }
            j = (j + 1);
        }
        i = (i + 1);
    }
    i = 0;
    while((i < 3)){
        j = 0;
        while((j < 3)){
            WriteLong((m3[(( i * 24 ) + (j * 8))>>3]));
            j = (j + 1);
        }
        WriteLine();
        i = (i + 1);
    }
}

