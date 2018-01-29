#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
using namespace std;
#define funcsize 20
// 程序支持的最大函数数量（包含主函数）
#define instr_max_size 20000
// 程序设计支持的最大三地址指令条数
#define blocksize 50
// 这里设置的是单个函数内的最多基本块数量

enum {add = 1, sub, mul, _div, mod, neg, cmpeq, cmple, cmplt,\
      br, blb, blbc, blbs, call, load, store, _move, read, write, wrl,\
      param, enter, entrypc, ret, nop};

struct FUNC{
    int bnum;
    int inst;
}ftag[funcsize];
// 这里存放了所有的函数相关数据

int btag[funcsize][blocksize];
// 这里存放的是每个block的相关信息

int vlive[instr_max_size];
int rflive[instr_max_size][2];
char looptype[instr_max_size];
int loopto[instr_max_size];
int loopfrom[instr_max_size];
// 该数组存放了所有虚拟寄存器的活跃性信息
// 用于辅助rtag实现中间变量消除和表达式合并
// 同时也包含了分支跳转信息

int type, offset;
int funcnow=0, blocknow, functotal;
string strtemp, name;

void ckvar(int type, int line){
    int num;
    if(type == 1){
        cin>>num;
        if(rflive[line][0] == 0)
            rflive[line][0]= num;
        else rflive[line][1] = num;
        vlive[num] = line;
    }
    else if(type == 6){
        cin>>name>>offset;
    }
    else cin>>strtemp;
}
// 用于虚拟寄存器的活跃性分析和参数、局部变量的信息记录

// 下面是扫描过程中的的处理函数
void get1(int opc, int line){
    cin>>type;
    ckvar(type, line);
    cin>>type;
    ckvar(type, line);
}
// add sub mul _div mod cmpeq cmple cmplt _move

void get2(int opc, int line){
    cin>>type;
    ckvar(type,line);
}
// neg load //

void get3(int opc, int line){
    int num;
    if(opc == ret)
        cin>>type>>num;
    else if(opc == br){
        cin>>type>>num;
        loopto[line] = num;
        loopfrom[num]=line;
        looptype[line]=1;
    }
    else if(opc != read){
        cin>>type;
        ckvar(type, line);
        if(opc == call)
            looptype[line]=3;
    }
}
// call read write br param ret //

void get4(int opc, int line){
    int inst;
    cin>>type;
    ckvar(type, line);
    if(opc == store){
        cin>>type;
        ckvar(type, line);
    }
    else {
        cin>>type>>inst;
        loopto[line] = inst;
        loopfrom[inst]=line;
        looptype[line]=2;
    }
}
//blbc blbs blb store //

void get6(int opc, int line){
    int lvbyte;
    cin>>type>>lvbyte;
    ftag[funcnow++].inst = line;
}
// enter //

void printcfg(int lines){
    int i,j, nextf, nextb;
    for(i=0;i<functotal;i++){
        cout<<"Function: "<<ftag[i].inst<<endl;
        if(i == functotal - 1) nextf = lines;
        else nextf = ftag[i+1].inst;
        blocknow = 0;
        for(j=ftag[i].inst;j<nextf;j++){
            if(j == ftag[i].inst)
                btag[i][blocknow++] = j;
            else if(loopfrom[j] != 0){
                if(btag[i][blocknow-1] == j)continue;
                btag[i][blocknow++] = j;
            }
            else if(loopto[j] != 0 && loopfrom[j] == 0)
                btag[i][blocknow++] = j+1;
            else if(looptype[j] == 3)
                btag[i][blocknow++] = j+1;
        }
        ftag[i].bnum = blocknow;
        cout<<"Basic blocks: ";
        for(j = 0;j<blocknow;j++){
            cout<<btag[i][j];
            if(j != blocknow - 1)cout<<' ';
        }
        cout<<endl<<"CFG:"<<endl;
        for(j = 0;j<blocknow;j++){
            cout<<btag[i][j]<<" ->";
            if(j != blocknow - 1){
                nextb = btag[i][j+1];
                switch(looptype[nextb-1]){
                case 0:
                case 3:cout<<' '<<nextb;break;
                case 1:cout<<' '<<loopto[nextb-1];break;
                case 2:
                    cout<<' '<<nextb<<' '<<loopto[nextb-1];
                    break;
                }
                cout<<endl;
            }
            else cout<<endl;
        }
    }
}

int main(){
    int lines,opc,i,option;
    cin>>lines;
    for(i=2; i<lines; i++){
        cin>>opc;
        switch(opc){
        case add: case sub: case mul: case _div:
        case mod: case cmpeq: case cmple:
        case cmplt: case _move:
            get1(opc, i);
            break;

        case neg: case load:
            get2(opc, i);
            break;

        case call: case read: case write:
        case param: case br: case ret:
            get3(opc, i);
            break;

        case blbc: case blbs: 
        case blb: case store:
            get4(opc, i);
            break;

        case enter:
            get6(opc, i);
            break;
        }
    }
    cin>>lines;
    functotal = funcnow;
    printcfg(lines);
    cin>>option;
    return 0;
}

