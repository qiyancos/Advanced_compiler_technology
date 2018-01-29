#include <stdio.h>
#include <string.h>

void get1(char* a){
    int n = strlen(a);
    if(a[0] == '('){
        a[0]='r';
        a[n-1]= 0;
        printf("%s", a);
    }
    else if(a[0] == '['){
        a[0]= ' ';
        a[n-1]= 0;
        printf("instr%s", a);
    }
    else printf("%s", a);
}

void get2(char* a){
    int n = strlen(a);
    if(a[0] == '('){
        a[0]=' ';
        a[n-1]= 0;
        printf("1%s", a);
    }
    else if(a[0] == '['){
        a[0]= ' ';
        a[n-1]= 0;
        printf("2%s", a);
    }
    else if(a[0] == 'F' && a[1] == 'P' && n == 2){
        printf("3 0");
    }
    else if(a[0] == 'G' && a[1] == 'P' && n == 2){
        printf("4 0");
    }
    else if(a[0] >= '0' && a[0] <= '9'){
        printf("5 %s", a);
    }
    else {
        printf("6 %s", a);
    }
}

int main()
{
    int choice;
    char a[1000];
    scanf("%d", &choice);
    switch(choice){
    case 1:
        scanf("%s", a);
        get1(a);
        break;
    case 2:
        scanf("%s", a);
        get2(a);
        break;
    }
    return 0;
}

