#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <string.h>
#include <algorithm>
using namespace std;
#define rtagset 10
// 类cache设计的rtag的分组数目
#define rtagsetsize 8
// 类cache设计的rtag的组相联度，即每组rtag数目
#define funcsize 20
// 程序支持的最大函数数量（包含主函数）
#define paramsize 10
// 程序设计的单个函数最大支持参数数目
#define localsize 50
// 程序设计的单个函数内最大定义的局部变量数目
#define globalsize 50
// 程序设计的最大支持的全局变量的数目
#define instr_max_size 20000
// 程序设计支持的最大三地址指令条数
#define maxadd 32768
// 这里设置了三地址码地址空间最高地址的位置
#define maxifnest 10
// 这里设置了if-else结构嵌套的最大深度限制

enum {add = 1, sub, mul, _div, mod, neg, cmpeq, cmple, cmplt,\
      br, blb, blbc, blbs, call, load, store, _move, read, write, wrl,\
      param, enter, entrypc, ret, nop};

struct LOCAL{
    int offset;
    int _size;
    string name;
}ltag[funcsize][localsize];
// 这里存放的是一个函数内部的所有局部变量信息
// 为了更好的还原数据，我们也会记录相应的局部变量名称
// 局部变量的区分是通过offset和实际长度决定的（可能是数组）

struct PARAM{
    int offset;
    string name;
}pintag[paramsize], ptag[funcsize][paramsize];
// 这里存放的是一个函数的参数信息
// pintag存放的是临时参数列表，里面只记录name

struct FUNC{
    int pnum;
    int lnum;
    int inst;
}ftag[funcsize];
// 这里存放了所有的函数相关数据

struct GLOBAL{
    int offset;
    int _size;
    string name;
}gtag[globalsize];
// 这里存放的是全局变量的相关信息
// 保留了全局变量的名字，所有数组均被转换为一维数组

struct REG{
    char flag;
    int rnum;
    string expr;
    char addsize;
    string arrname;
}side_rtag[rtagsetsize], main_rtag[rtagset * rtagsetsize];
// 这里存放的是虚拟寄存器的保留表达式信息
// 用来消除过多的虚拟寄存器作为中间变量的冗余
// 而尽量将表达式合并成一个不包含中间变量的形式

int vlive[instr_max_size];
int rflive[instr_max_size][2];
int loopto[instr_max_size];
int loopfrom[instr_max_size];
// 该数组存放了所有虚拟寄存器的活跃性信息
// 用于辅助rtag实现中间变量消除和表达式合并
// 同时也包含了分支跳转信息

char flag_main=0, flag_loop=0, midnum[20];
int type, offset, pnow, ifed[maxifnest], ifednow;
int funcnow=0, functotal, tabcounter = 0, var[2];
string strtemp, name, strtempin;
REG rtagtemp;

bool cmpp(PARAM a, PARAM b){
    return a.offset > b.offset;
}
bool cmpl(LOCAL a, LOCAL b){
    return a.offset < b.offset;
}
bool cmpg(GLOBAL a, GLOBAL b){
    return a.offset > b.offset;
}

void ckdigit(string a){
    int strl = a.length();
    var[0] = 1;
    if(a[0] != '-' && (a[0] < '0' || a[0] > '9')) {
        var[0] = 0;
        var[1] = 1;
        return;
    }
    for(int i=1;i<strl;i++){
        if(a[i] < '0' || a[i] > '9'){
            var[0]=0;
            var[1]=1;
            break;
        }
    }
    if(var[0]) var[1] = atoi(a.c_str());
    //cout<<"Check "<<var[0]<<':'<<var[1]<<endl;
}
// 主要用于字符串是否为数字的检查

void updatesize(){
    int i,j;
    for(i=0;i<functotal;i++){
        sort(ltag[i], ltag[i] + localsize - 1, cmpl);
        for(j=0;j<localsize;j++){
            if(ltag[i][j].offset == 0)break;
            else
                ltag[i][j]._size = (ltag[i][j+1].offset\
                - ltag[i][j].offset)>>3;
        }
    }
    sort(gtag, gtag + globalsize - 1, cmpg);
    for(j=0;j<globalsize;j++){
        if(gtag[j].offset == 0)break;
        else if(j == 0) {
            gtag[j]._size = (maxadd - gtag[j].offset)>>3;
        }
        else gtag[j]._size = (gtag[j-1].offset - gtag[j].offset)>>3;
    }
}
// 对所有变量容器进行排序，使我们可以根据offset快速对应到变量
// 同时更新所有变量的长度信息来区分数组，方便及进行数据定义声明

void printtab(){
    int i=tabcounter;
    while(i--){
        cout<<"    ";
    }
}

void newgvar(){
    int strl;
    for(int i=0;i<globalsize;i++){
        if(gtag[i].offset != 0){
            if(gtag[i].offset == offset)return;
        }
        else {
            strl = name.size();
            name.erase(strl-5, 5);
            gtag[i].name=name;
            gtag[i].offset=offset;
            return;
        }
    }
}
// 扫描过程中对全局变量的添加

void newlvar(int flag){
    int strl;
    if(offset > 0){
        int pnum = (offset >> 3) - 1;
        if(ptag[funcnow][pnum].offset != 0) return;
        else{
            ptag[funcnow][pnum].name=name;
            ptag[funcnow][pnum].offset=offset;
        }
    }
    else {
        for(int i=0; i<localsize; i++){
            if(ltag[funcnow][i].offset != 0){
                if(ltag[funcnow][i].offset == offset)return;
            }
            else{
                if(flag == 1){
                    strl = name.size();
                    name.erase(strl-5, 5);
                }
                ltag[funcnow][i].name=name;
                ltag[funcnow][i].offset=offset;
                break;
            }
        }
    }
}
// 增加新的局部变量或者函数参数，包括数组和非数组

void ckvar(int type, int line){
    int num,strl;
    if(type == 1){
        cin>>num;
        if(rflive[line][0] == 0)
            rflive[line][0]= num;
        else rflive[line][1] = num;
        vlive[num] = line;
    }
    else if(type == 6){
        cin>>name>>offset;
        strl = name.size();
        if(offset < (paramsize << 4)){
            if(strl > 5 && name.substr(strl-5, 5) == "_base")
                newlvar(1);
            else newlvar(2);
        }
    }
    else cin>>strtemp;
}
// 用于虚拟寄存器的活跃性分析和参数、局部变量的信息记录

REG* findrtag(int rnum, int put){
    int setst, seted, i;
    setst = (rnum % rtagset) * rtagsetsize;
    seted = setst + rtagsetsize;
    for(i = setst; i<seted; i++){
        if(put == 0 && main_rtag[i].rnum == rnum && main_rtag[i].flag == 1)
            return &main_rtag[i];
        else if(put == 1 && main_rtag[i].flag == 0)
            return &main_rtag[i];
    }
    for(i=0; i<rtagsetsize; i++){
        if(put == 0 && side_rtag[i].rnum == rnum && side_rtag[i].flag == 1)
            return &side_rtag[i];
        else if(put == 1 && side_rtag[i].flag == 0)
            return &side_rtag[i];
    }
}
// 查询给定的寄存器编号对应的rtag位置

void freertag(int line){
    int i=2, clearnum;
    REG* getnow;
    while(i--){
        clearnum = rflive[line][1-i];
        if(clearnum == 0)break;
        else {
            getnow = findrtag(clearnum, 0);
            (*getnow).flag = 0;
        }
    }
}
// 释放rtag中不再使用的寄存器的rtag

void printrtag(REG putin){
    cout<<"//"<<"check putrtag"<<endl;
    cout<<"//"<<(int)putin.flag<<':'<<putin.rnum<<endl;
    cout<<"//"<<putin.expr<<endl;
    cout<<"//"<<(int)putin.addsize<<':'<<putin.arrname<<endl;
}
// debug用函数

void putrtag(REG putin){
    if(vlive[putin.rnum] != 0){
        REG* getnow = findrtag(putin.rnum, 1);
        *getnow = putin;
        //printrtag(*getnow);
    }
}
// 放置每个寄存器对应的表达式信息

int findsize(int offset){
    if(offset > (paramsize << 4)){
        for(int i=0; i<globalsize; i++){
            if(gtag[i].offset < offset)return 0;
            else if(gtag[i].offset == offset)
                return gtag[i]._size;
        }
    }
    else return 2;
}
// 获取一个变量的大小信息

REG newrtag(int type, int line){
    int num;
    if(type == 1){
        cin>>num;
        rtagtemp = *(findrtag(num, 0));
        ckdigit(rtagtemp.expr);
        if(!var[0] && rtagtemp.expr.find(' ') != -1){
            strtemp = '(';
            strtemp += rtagtemp.expr;
            strtemp += ')';
            rtagtemp.expr = strtemp;
        }
    }
    else if(type == 6){
        cin>>strtemp;
        int strl = strtemp.size();
        if(strl > 5 && strtemp.substr(strl-5, 5) == "_base"){
            cin>>num;
            rtagtemp.addsize = findsize(num);
            rtagtemp.arrname = strtemp.substr(0, strl-5);
            rtagtemp.expr = "0";
        }
        else if(strl > 7 && strtemp.substr(strl-7, 7) == "_offset"){
            cin>>strtemp;
            rtagtemp.addsize = 0;
            rtagtemp.expr = strtemp;
        }
        else{
            cin>>num;
            rtagtemp.expr = strtemp;
            rtagtemp.addsize = 0;
        }
    }
    else if(type == 5){
        cin>>strtemp;
        rtagtemp.expr = strtemp;
        rtagtemp.addsize = 0;
    }
    else {
        cin>>strtemp;
        rtagtemp.expr = "0";
    }
    rtagtemp.flag = 1;
    return rtagtemp;
}
// 这里进行主要是表达式的合并转换
// 包括首次出现的常数或者基本变量的标准表示形式生成

void printglobal(){
    for(int i=0;i<globalsize;i++){
        if(gtag[i]._size !=0){
            cout<<"long "<<gtag[i].name;
            if(gtag[i]._size > 1)
                cout<<'['<<gtag[i]._size<<']';
            cout<<';'<<endl;
        }
    }
}
// 打印全局变量的定义声明

// 下面是扫描过程中的的处理函数
void get1(int flag, int opc, int line){
    int flagc = 0, flag0 = 0;
    if(flag){
        cin>>type;
        ckvar(type, line);
        cin>>type;
        ckvar(type, line);
        if(type == 4) newgvar();
        else if(type == 3) newlvar(2);
    }
    else {
        if(opc == _move){
            printtab();
            cin>>type;
            rtagtemp = newrtag(type, line);
            strtempin = rtagtemp.expr;
            cin>>type;
            rtagtemp = newrtag(type, line);
            cout<<rtagtemp.expr<<" = ";
            cout<<strtempin<<';'<<endl;
            strtemp = rtagtemp.expr;
            rtagtemp.expr = strtemp;
            rtagtemp.rnum = line;
            putrtag(rtagtemp);
        }
        else {
            char flagtemp;
            string arrname;
            cin>>type;
            rtagtemp = newrtag(type, line);
            flagtemp = rtagtemp.addsize;
            arrname = rtagtemp.arrname;
            ckdigit(rtagtemp.expr);
            if(var[0]) {
                if(!var[1]) flag0 = 1;
                flagc = 1;
            }
            strtempin = rtagtemp.expr;
            cin>>type;
            rtagtemp = newrtag(type, line);
            ckdigit(rtagtemp.expr);
            if(flagc && var[0]){
                flagc = 2;
                switch(opc){
                case add:var[1] = atoi(strtempin.c_str()) + var[1];break;
                case sub:var[1] = atoi(strtempin.c_str()) - var[1];break;
                case mul:var[1] = atoi(strtempin.c_str()) * var[1];break;
                case _div:var[1] = atoi(strtempin.c_str()) / var[1];break;
                case mod:var[1] = atoi(strtempin.c_str()) % var[1];break;
                case cmpeq:var[1] = atoi(strtempin.c_str()) == var[1];break;
                case cmplt:var[1] = atoi(strtempin.c_str()) < var[1];break;
                case cmple:var[1] = atoi(strtempin.c_str()) <= var[1];break;
                }
                sprintf(midnum, "%d", var[1]);
                strtempin = midnum;
            }
            else if(flag0 || (var[0] && !var[1])){
                switch(opc){
                case add:
                    if(strtempin == "0") {
                        strtempin = rtagtemp.expr;
                        strtempin[0] = ' ';
                        strtempin[rtagtemp.expr.length()-1] = ' ';
                    }
                    break;
                case sub:
                    if(strtempin == "0") {
                        strtempin = '-';
                        strtempin += rtagtemp.expr;
                    }
                    break;
                case mul: case _div: case mod:
                    strtempin = "0";break;
                case cmpeq:
                    strtempin += " == ";
                    strtempin += rtagtemp.expr;
                    break;
                case cmplt:
                    strtempin += " < ";
                    strtempin += rtagtemp.expr;
                    break;
                case cmple:
                    strtempin += " <= ";
                    strtempin += rtagtemp.expr;
                    break;
                }
            }
            else {
                switch(opc){
                case add:strtempin += " + ";break;
                case sub:strtempin += " - ";break;
                case mul:strtempin += " * ";break;
                case _div:strtempin += " / ";break;
                case mod:strtempin += " % ";break;
                case cmpeq:strtempin += " == ";break;
                case cmplt:strtempin += " < ";break;
                case cmple:strtempin += " <= ";break;
                }
                strtempin += rtagtemp.expr;
            }
            if(rtagtemp.addsize == 0) rtagtemp.arrname = arrname;
            if(flagtemp > 1 || rtagtemp.addsize > 1)
                rtagtemp.addsize = 2;
            else if(flagtemp == 1 || rtagtemp.addsize == 1)
                rtagtemp.addsize = 1;
            else rtagtemp.addsize = 0;
            rtagtemp.expr = strtempin;
            rtagtemp.rnum = line;
            rtagtemp.flag = 1;
            putrtag(rtagtemp);
        }
    }
}
// add sub mul _div mod cmpeq cmple cmplt _move

void get2(int flag, int opc, int line){
    if(flag){
        cin>>type;
        ckvar(type,line);
    }
    else {
        cin>>type;
        if(opc == neg){
            rtagtemp = newrtag(type, line);
            strtemp = "-";
            strtemp += rtagtemp.expr;
            rtagtemp.expr = strtemp;
            putrtag(rtagtemp);
        }
        else {
            rtagtemp = newrtag(type, line);
            strtemp = rtagtemp.arrname;
            if(rtagtemp.addsize != 1){
                strtemp += "[";
                ckdigit(rtagtemp.expr);
                if(!var[0]){
                    strtemp += rtagtemp.expr;
                    strtemp += ">>3]";
                }
                else {
                    sprintf(midnum, "%d", var[1]>>3);
                    strtemp += midnum;
                    strtemp += ']';
                }
            }
            rtagtemp.expr = strtemp;
            rtagtemp.addsize = 0;
            rtagtemp.rnum = line;
            putrtag(rtagtemp);
        }
    }
}
// neg load //

void get3(int flag, int opc, int line){
    int num, i;
    if(flag){
        if(opc == ret){
            cin>>type>>num;
            ftag[funcnow++].pnum=num>>3;
        }
        else if(opc == br){
            cin>>type>>num;
            loopto[line] = num;
            loopfrom[num]=line;
        }
        else if(opc != read){
            cin>>type;
            ckvar(type, line);
        }
    }
    else {
        switch(opc){
        case param:
            cin>>type;
            if(type == 1){
                cin>>num;
                pintag[pnow++].name = (*findrtag(num, 0)).expr;
            }
            else if(type == 6)
                cin>>pintag[pnow++].name>>strtemp;
            else if(type == 5)
                cin>>pintag[pnow++].name;
            break;
        case call:
            printtab();
            cin>>strtemp>>num;
            for(i=0;i<functotal;i++){
                if(ftag[i].inst == num){
                    cout<<"func_"<<i<<'(';
                    break;
                }
            }
            if(pnow != 0){
                for(i = 0; i<pnow;i++){
                    cout<<pintag[i].name;
                    if(i != pnow -1)cout<<", ";
                }
            }
            pnow=0;
            cout<<");"<<endl;
            break;
        case ret:
            tabcounter --;
            printtab();
            cin>>strtemp>>strtemp;
            cout<<'}'<<endl<<endl;;
            funcnow ++;
            break;
        case read:
            printtab();
            char tempstr[10];
            rtagtemp.flag = 1;
            rtagtemp.addsize = 0;
            rtagtemp.rnum = line;
            rtagtemp.expr = "temp_";
            sprintf(tempstr, "%d", line);
            strtemp = tempstr;
            rtagtemp.expr += strtemp;
            cout<<"ReadLong(temp_"<<strtemp;
            cout<<");"<<endl;
            putrtag(rtagtemp);
            break;
        case write:
            printtab();
            cin>>type;
            rtagtemp = newrtag(type,line);
            cout<<"WriteLong("<<rtagtemp.expr;
            cout<<");"<<endl;
            break;
        case br:
            cin>>strtemp>>num;
            if(loopto[line] < line){
                tabcounter--;
                printtab();
                cout<<'}'<<endl;
            }
            else {
                tabcounter --;
                printtab();
                cout<<'}'<<endl;
                printtab();
                cout<<"else {"<<endl;
                tabcounter++;
                ifed[ifednow++] = num - 1;
            }
            break;
        }
    }
}
// call read write br param ret //

void get4(int flag, int opc, int line){
    int inst;
    if(flag){
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
        }
    }
    else {
        if(opc == blbc || opc == blbs || opc == blb){
            printtab();
            cin>>type;
            rtagtemp = newrtag(type, line);
            cin>>type>>inst;
            if(flag_loop) {
                cout<<"while(";
                flag_loop--;
            }
            else {
                cout<<"if(";
                if(loopto[inst-1] < inst - 1)
                    ifed[ifednow++] = inst - 1;
            }
            if(opc == blbs || opc == blb)
                cout<<"!("<<rtagtemp.expr<<')';
            else cout<<rtagtemp.expr;
            cout<<"){"<<endl;
            tabcounter++;
        }
        else if(opc == store){
            printtab();
            cin>>type;
            rtagtemp = newrtag(type, line);
            strtempin = rtagtemp.expr;
            cin>>type;
            rtagtemp = newrtag(type, line);
            cout<<rtagtemp.arrname;
            if(rtagtemp.addsize != 1){
                ckdigit(rtagtemp.expr);
                if(!var[0]) cout<<"["<<rtagtemp.expr<<">>3]";
                else cout<<'['<<(var[1]>>3)<<']';
            }
            cout<<" = "<<strtempin<<';'<<endl;
        }
    }
}
//blbc blbs store //

void get5(int flag, int opc, int line){
    if(opc == entrypc && !flag)flag_main=1;
    else if(!flag && opc == wrl){
        printtab();
        cout<<"WriteLine();"<<endl;
    }
}
// enterypc nop wrl //

void get6(int flag, int opc, int line){
    int lvbyte, i;
    cin>>type>>lvbyte;
    if(flag){
        ftag[funcnow].lnum=lvbyte>>3;
        ftag[funcnow].inst=line;
        // 记录函数的局部变量数目和指令位置标记
    }
    else {
        if(flag_main) {
            printtab();
            cout<<"void main(){"<<endl;
            flag_main = 0;
            tabcounter++;
        }
        else {
            printtab();
            cout<<"void func_"<<funcnow<<'(';
            if(ftag[funcnow].pnum != 0){
                for(i = ftag[funcnow].pnum;i>0;i--){
                    cout<<"long ";
                    if(ptag[funcnow][i].offset != 0)
                        cout<<ptag[funcnow][i].name;
                    else cout<<"param_"<<(ftag[funcnow].pnum - i + 1);
                    if(i != 1) cout<<", ";
                }
            }
            cout<<"){"<<endl;
            tabcounter++;
        }
        if(ftag[funcnow].lnum != 0){
            for(i=0;i<ftag[funcnow].lnum;i++){
                if(ltag[funcnow][i].offset == 0)break;
                printtab();
                cout<<"long "<<ltag[funcnow][i].name;
                if(ltag[funcnow][i]._size > 1)
                    cout<<'['<<ltag[funcnow][i]._size<<']';
                cout<<";"<<endl;
            }
        }
        pnow=0;
    }
}
// enter //

int main(){
    int lines,opc,i,j;
    int temp=2;
    while(temp--){
        if(!temp){
            cout<<endl;
            functotal = funcnow;
            funcnow = 0;
            ifednow = 0;
            updatesize();
            printglobal();
            memset(ifed, 0, sizeof(ifed));
            cout<<endl;
        }
        cin>>lines;
        for(i=2; i<lines; i++){
            cin>>opc;
            switch(opc){
            case add: case sub: case mul: case _div:
            case mod: case cmpeq: case cmple:
            case cmplt: case _move:
                get1(temp, opc, i);
                break;

            case neg: case load:
                get2(temp, opc, i);
                break;

            case call: case read: case write:
            case param: case br: case ret:
                get3(temp, opc, i);
                break;

            case blbc: case blbs:
            case store: case blb:
                get4(temp, opc, i);
                break;

            case wrl: case entrypc: case nop:
                get5(temp,opc, i);
                break;
            case enter:
                get6(temp, opc, i);
                break;
            }
            if(!temp)freertag(i);
            if(loopfrom[i] > i) flag_loop++;
            j = maxifnest;
            while(j--){
                if(ifed[ifednow - 1] && i == ifed[ifednow - 1]){
                tabcounter--;
                printtab();
                cout<<'}'<<endl;
                ifednow--;
                }
                else break;
            }
        }
        cin>>lines;
    }
    return 0;
}

