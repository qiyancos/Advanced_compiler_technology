#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <errno.h>
using namespace std;
#define crtagsize 20
// 存放保存着常量的虚拟寄存器的crtag的数目
#define globalsize 512
// 确定程序允许的全局变量最大容量（8Byte为单位）
#define localsize 512
// 程序设计的单个函数局部变量的最大容量（8Byte为单位）
#define paramsize 16
// 程序设计的单个函数的最大参数数量
#define maxbfrom 10
// 表示一个基本块最多有多少个入口
#define funcsize 20
// 程序支持的最大函数数量（包含主函数）
#define instr_max_size 20000
// 程序设计支持的最大三地址指令条数
#define blocksize 100
// 这里设置的是单个函数中最多的基本块数量
#define maxadd 32768
// 这里设置的是CSC存储结构中最高GP偏移地址

enum {add = 1, sub, mul, _div, mod, neg, cmpeq, cmple, cmplt,\
      br, blb, blbc, blbs, call, load, store, _move, read, write, wrl,\
      param, enter, entrypc, ret, nop};

struct FUNC{
    int bnum;
    int inst;
}ftag[funcsize];
// 这里存放了所有的函数相关数据

struct LOCAL{
    int offset;
    int _size;
}ltag[funcsize][localsize];
// 这里存放的是一个函数内部的所有局部变量信息
// 为了更好的还原数据，我们也会记录相应的局部变量名称
// 局部变量的区分是通过offset和实际长度决定的（可能是数组）

struct GLOBAL{
    int offset;
    int _size;
}gtag[globalsize];
// 这里存放的是全局变量的相关信息
// 保留了全局变量的名字，所有数组均被转换为一维数组

struct LCONST{
    char flag=0;
    int constin;
}tempcltag[localsize+globalsize];
// 这里记录常量传播的情况

struct BLOCK{
    int start;
    int _end;
    // 基本块指令的范围
    char bfromnow;
    // 前继基本块数目
    int bfrom[maxbfrom];
    // 前继基本块的编号
    int bto[2];
    // 后继基本块编号
    LCONST gencltag[localsize+globalsize];
    // 基本块内生成常量传播记录
    LCONST outcltag[localsize+globalsize];
    // 输出的常量传播记录
    LCONST incltag[localsize+globalsize];
    // 输入的常量传播记录
    char kill[localsize+globalsize];
    // 基本块内杀死的变量集合
}btag[funcsize][blocksize];
// 这里设置的是基本块的数据，包含跳转位置
// 每个基本块都保留了一个开始位置以及基本块
// 最后输出的常量定义相关信息用于进行常量传播

struct CREG{
    char flag;
    int rnum;
    int constin;
    char addrflag;
    int base;
}crtag[crtagsize];
// 这里存放的是虚拟寄存器的常数信息
// 用来消除过多的虚拟寄存器作为中间变量的冗余
// 而尽量将表达式合并成一个不包含中间变量的形式

int vlive[instr_max_size];
int rflive[instr_max_size][2];
// 该数组存放了所有虚拟寄存器的活跃性信息
// 用于辅助crtag实现中间变量消除和表达式合并
// rflive表示该位置最后活跃的信息，用于腾空不用的crtag
int loopto[instr_max_size];
int loopfrom[instr_max_size];
int looptype[instr_max_size];
// 同时也包含了分支跳转信息
// loopto表示要跳转到哪里
// loopfrom表示要从哪跳过来
// looptype表示跳转类型（条件/非条件）

char flag_main=0, flag_loop=0;
int type, offset, blocknow, glbnum;
int funcnow=0, functotal;
string strtemp, name, strtempin, errorinfo;
char trash[100];
CREG crtagtemp;
LCONST cltagtemp;

int tagnum(int offset){
    if(offset > maxadd - (globalsize << 4))
        return ((maxadd-offset)>>3)+localsize;
    else if(offset < 0) return (-offset)>>3;
}
// 这里处理的是从offset到标号的映射处理

bool cmpl(LOCAL a, LOCAL b){
    return a.offset < b.offset;
}
bool cmpg(GLOBAL a, GLOBAL b){
    return a.offset > b.offset;
}

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

void newgvar(){
    if(offset < maxadd - (globalsize << 4) && offset > 0){
        errorinfo = "Fatal Error: Parameter <globalsize> Not Big Enough!";
        perror(errorinfo.c_str());
        exit(0);
    }
    for(int i=0;i<globalsize;i++){
        if(gtag[i].offset != 0){
            if(gtag[i].offset == offset)return;
        }
        else {
            gtag[i].offset=offset;
            return;
        }
    }
}
// 扫描过程中对全局变量的添加

void newlvar(int flag){
    if(offset < - (localsize << 3) - 8){
        errorinfo = "Fatal Error: Parameter <localsize> Not Big Enough!";
        perror(errorinfo.c_str());
        exit(0);
    }
    if(offset < 0){
        for(int i=0; i<localsize; i++){
            if(ltag[funcnow-1][i].offset != 0){
                if(ltag[funcnow-1][i].offset == offset)return;
            }
            else{
                ltag[funcnow-1][i].offset=offset;
                break;
            }
        }
    }
}
// 增加新的局部变量或者函数参数，包括数组和非数组

int findsize(int offset){
    if(offset > maxadd - (globalsize << 4)){
        for(int i=0; i<globalsize; i++){
            if(gtag[i].offset < offset)return 0;
            else if(gtag[i].offset == offset)
                return gtag[i]._size;
        }
    }
    else if(offset < 0){
        for(int i=0; i<localsize; i++){
            if(ltag[funcnow][i].offset > offset)return 0;
            else if(ltag[funcnow][i].offset == offset)
                return ltag[funcnow][i]._size;
        }
    }
}
// 获取一个变量的大小信息

void addkill(int offset){
    //cout<<"Add Kill: "<<funcnow<<':'<<blocknow<<"->"<<offset<<endl;
    btag[funcnow][blocknow].kill[tagnum(offset)] = 1;
}
// 用于添加基本块内的kill信息

LCONST ckcltag(int flag, int offset, int constin){
    LCONST ret;
    int i = tagnum(offset);
    //if(flag == 1)cout<<"// Find cltag "<<offset<<endl;
    //if(flag == 2)cout<<"// Add cltag "<<offset<<":"<<constin<<endl;
    //else if(flag == 3)cout<<"// Kill cltag "<<offset<<endl;
    if(flag == 1){
        //cout<<"// Got it "<<offset<<':'<<tempcltag[i].constin<<endl;
        return tempcltag[i];
    }
    else if(flag == 2){
        tempcltag[i].flag = 1;
        tempcltag[i].constin = constin;
        return ret;
    }
    else tempcltag[i].flag = 0;
    return ret;
}
// 这里是块内处理以及块间处理过程中kill操作
// 也可以被用来进行查找操作；
// flag=1, 查找并返回一个cltag结构
// flag=2, 进行添加或者替换操作
// flag=3, 进行kill操作

CREG* findcrtag(int rnum, int put){
    for(int i = 0; i<crtagsize; i++){
        if(!put && crtag[i].flag == 1 && crtag[i].rnum == rnum)
            return &crtag[i];
        else if(put && crtag[i].flag == 0)
            return &crtag[i];
    }
    crtagtemp.flag = 0;
    return &crtagtemp;
}
// 查询给定的寄存器编号对应的crtag位置
// put=1表示进行空闲添加位置查找
// put=0表示进行已存在的位置进行查找

void freecrtag(int line){
    int i=2, clearnum;
    CREG* getnow;
    while(i--){
        clearnum = rflive[line][1-i];
        if(clearnum == 0)break;
        else {
            getnow = findcrtag(clearnum, 0);
            (*getnow).flag = 0;
        }
    }
}
// 释放crtag中不再使用的寄存器的crtag

void printcrtag(CREG putin){
    cout<<"//"<<"check putcrtag"<<endl;
    cout<<"//"<<(int)putin.flag<<':'<<putin.rnum<<endl;
    cout<<"//"<<(int)putin.constin<<':'<<putin.base<<endl;
}
// debug用函数

void putcrtag(CREG putin){
    if(vlive[putin.rnum] != 0){
        CREG* getnow = findcrtag(putin.rnum, 1);
        *getnow = putin;
        //printcrtag(*getnow);
    }
}
// 放置每个寄存器对应的表达式信息

void newcrtag(int typein, int line){
    LCONST feedback;
    char flagin;
    if(typein == 1){
        cin>>glbnum;
        crtagtemp = *(findcrtag(glbnum, 0));
        //cout<<"Find "<<(int)crtagtemp.flag<<endl;
    }
    else if(typein == 6){
        cin>>strtemp>>offset;
        int strl = strtemp.size();
        if(strl > 5 && strtemp.substr(strl-5, 5) == "_base"){
            crtagtemp.constin = offset;
            flagin = 1;
            crtagtemp.addrflag = 1;
            crtagtemp.rnum = 0;
            crtagtemp.flag = 1;
        }
        else if(strl > 7 && strtemp.substr(strl-7, 7) == "_offset"){
            crtagtemp.constin = offset;
            crtagtemp.addrflag = 0;
            crtagtemp.base = 0;
            crtagtemp.rnum = 0;
            crtagtemp.flag = 1;
        }
        else{
            feedback = ckcltag(1, offset, 0);
            if(feedback.flag){
                crtagtemp.constin = feedback.constin;
                crtagtemp.addrflag = 0;
                crtagtemp.rnum = line;
                crtagtemp.flag = 1;
            }
            else crtagtemp.flag = 0;
        }
        if(flagin) crtagtemp.base = offset;
        else crtagtemp.base = 0;
    }
    else if(typein == 5){
        cin>>glbnum;
        crtagtemp.constin = glbnum;
        crtagtemp.flag = 1;
        crtagtemp.base = 0;
        crtagtemp.addrflag = 0;
    }
    else {
        cin>>glbnum;
        if(typein == 4 || typein == 3){
            crtagtemp.constin = 0;
            crtagtemp.base = 0;
            crtagtemp.addrflag = 0;
            crtagtemp.flag = 1;
        }
        else crtagtemp.flag = 0;
    }
}
// 这里进行主要是表达式的合并转换
// 包括首次出现的常数或者基本变量的标准表示形式生成

void ckvar(int typein, int line){
    int num;
    if(typein == 1){
        cin>>num;
        if(rflive[line][0] == 0)
            rflive[line][0]= num;
        else rflive[line][1] = num;
        vlive[num] = line;
    }
    else if(typein == 6){
        cin>>name>>offset;
        int strl = name.length();
        if(offset < 0 && strl > 5 && name.substr(strl-5, 5) == "_base")
            newlvar(1);
        else if(offset > (paramsize << 4))newgvar();
    }
    else cin>>strtemp;
}
// 用于虚拟寄存器的活跃性分析和参数、局部变量的信息记录

int findblock(int funcnum, int instpos){
    for(int i=0;i<ftag[funcnum].bnum;i++){
        if(btag[funcnum][i].start == instpos)
            return i;
    }
    return -1;
}
// 用于寻找一个块的编号，从而快速找到一个块的位置

void getcfg(int lines){
    int i,j, nextf, nextb;
    memset(btag, 0, sizeof(btag));
    for(i=0;i<functotal;i++){
        if(i == functotal - 1) nextf = lines;
        else nextf = ftag[i+1].inst;
        blocknow = 0;
        for(j=ftag[i].inst;j<nextf;j++){
            if(j == ftag[i].inst)
                btag[i][blocknow++].start = j;
            else if(loopfrom[j] != 0){
                if(btag[i][blocknow-1].start == j)continue;
                btag[i][blocknow++].start = j;
            }
            else if(loopto[j] != 0 && loopfrom[j] == 0)
                btag[i][blocknow++].start = j+1;
            else if(looptype[j] == 3)
                btag[i][blocknow++].start = j+1;
        }
        // 进行基本块起始位置敲定
        ftag[i].bnum = blocknow;
        for(j = 0;j<blocknow;j++){
            if(j != blocknow - 1){
                int nextbnum;
                nextb = btag[i][j+1].start;
                btag[i][j]._end = nextb;
                //cout<<i<<':'<<j<<"->"<<btag[i][j]._end<<endl;
                switch(looptype[nextb-1]){
                case 0:
                case 3:
                    btag[i][j].bto[0] = j+1;
                    btag[i][j].bto[1] = -1;
                    btag[i][j+1].bfrom[btag[i][j+1].bfromnow++] = j;
                    break;
                case 1:
                    nextbnum = findblock(i, loopto[nextb-1]);
                    btag[i][j].bto[0] = nextbnum;
                    btag[i][j].bto[1] = -1;
                    btag[i][nextbnum].bfrom[btag[i][nextbnum].bfromnow++] = j;
                    break;
                case 2:
                    nextbnum = findblock(i, loopto[nextb-1]);
                    btag[i][j].bto[0] = j+1;
                    btag[i][j].bto[1] = nextbnum;
                    btag[i][j+1].bfrom[btag[i][j+1].bfromnow++] = j;
                    btag[i][nextbnum].bfrom[btag[i][nextbnum].bfromnow++] = j;
                    break;
                }
            }
            else {
                btag[i][j].bto[0] = -1;
                btag[i][j].bto[1] = -1;
                if(i+1 != functotal)btag[i][j]._end = ftag[i+1].inst;
                else btag[i][j]._end = lines;
                //cout<<i<<':'<<j<<"->"<<btag[i][j]._end<<endl;
            }
        }
    }
}
// 生成程序的CFG相关信息并存储起来

void mergeck(int k, int i, LCONST in){
    if(k == 0) tempcltag[i] = in;
    else if(in.flag != tempcltag[i].flag)
        tempcltag[i].flag = 0;
    else if(in.flag && tempcltag[i].flag && tempcltag[i].constin != in.constin){
        tempcltag[i].flag = 0;
        //cout<<"Change:"<<offset<<endl;
    }
}
// 这里主要进行的是常量传播的交汇操作

char procout(int fnum, int bnum){
    int i;
    char flagret = 0;
    for(i=0;i<localsize+globalsize;i++){
        if(btag[fnum][bnum].incltag[i].flag && !btag[fnum][bnum].kill[i])
            tempcltag[i] = btag[fnum][bnum].incltag[i];
    }
    // TEMP = IN - kill
    for(i=0;i<localsize+globalsize;i++){
        if(btag[fnum][bnum].gencltag[i].flag){
            tempcltag[i] = btag[fnum][bnum].gencltag[i];
            if(!tempcltag[i].flag) flagret = 1;
        }
    }
    memcpy(btag[fnum][bnum].outcltag, tempcltag, sizeof(tempcltag));
    // TEMP = GEN并(IN - kill)
    // OUT = TEMP
    return flagret;
}
// 这里进行进行块内从in到out的变化处理

void mergeblockin(){
    int i, j, k, y;
    int counts=0;
    char changeflag;
    //cout<<"Mergein"<<endl;
    for(i=0;i<functotal;i++){
        changeflag = 0;
        //cout<<"===================="<<endl;
        for(j=0;j<ftag[i].bnum;j++){
            //cout<<"Deal with "<<i<<' '<<j<<endl;
            memset(tempcltag, 0, sizeof(tempcltag));
            for(k=0;k<btag[i][j].bfromnow;k++){
                //cout<<"Check bfrom "<<(k+counts)<<'-'<<btag[i][j].bfrom[k]<<endl;
                for(y=0;y<localsize+globalsize;y++)
                    mergeck(counts+k, y, btag[i][btag[i][j].bfrom[k]].outcltag[y]);
            }
            // TEMP = 交汇所有前继
            //cout<<"After mergein:"<<(int)changeflag<<endl;
            for(y=0;y<localsize+globalsize;y++){
                if(tempcltag[y].flag && !btag[i][j].incltag[y].flag){
                    changeflag = 1;
                    //cout<<"Warning!"<<y<<endl;
                    btag[i][j].incltag[y] = tempcltag[y];
                }
            }
            // IN = TEMP
            memset(tempcltag, 0, sizeof(tempcltag));
            if(procout(i, j)) changeflag ++;
            //cout<<"After procout:"<<(int)changeflag<<endl;
        }
        if(changeflag){
            i--;
            counts++;
        }
        else counts = 0;
    }
}
// 该函数主要被用来进行基于CFG的常量传播信息合并处理，
// 最后的结果的是每个块依据CFG的常量传播输入情况

// 下面是扫描过程中的的处理函数
void get1(int flag, int opc, int line){
    char addrflagt = 0;
    if(flag == 2){
        cin>>type;
        ckvar(type, line);
        cin>>type;
        ckvar(type, line);
    }
    else if(flag < 2){
        if(opc == _move){
            int num, type1;
            cin>>type1;
            if(type1 < 5){
                if(type1 == 1){
                    //cout<<"Check "<<endl;
                    newcrtag(type1, line);
                    if(!flag) cout<<opc;
                    if(crtagtemp.flag){
                        //cout<<"Check 1"<<endl;
                        cin>>type>>strtemp>>offset;
                        ckcltag(2, offset, crtagtemp.constin);
                        crtagtemp.rnum = line;
                        if(!flag) cout<<" 5 "<<crtagtemp.constin;
                        putcrtag(crtagtemp);
                    }
                    else {
                        //cout<<"Check 2"<<endl;
                        cin>>type>>strtemp>>offset;
                        if(!flag) cout<<" 1 "<<glbnum;
                        if(flag == 1)addkill(offset);
                        ckcltag(3, offset, 0);
                    }
                    if(!flag) cout<<" 6 "<<strtemp<<' '<<offset<<endl;
                }
                else {
                    gets(trash);
                    if(!flag) printf("%s\n", trash);
                }
            }
            else if(type1 > 4){
                if(type1 == 5){
                    cin>>num>>type>>strtemp>>offset;
                    if(!flag) {
                        cout<<opc<<" 5 "<<num<<" 6 ";
                        cout<<strtemp<<' '<<offset<<endl;
                    }
                    ckcltag(2, offset, num);
                    crtagtemp.constin = num;
                    crtagtemp.rnum = line;
                    crtagtemp.flag = 1;
                    putcrtag(crtagtemp);
                }
                else if(type1 == 6){
                    cin>>strtemp>>offset;
                    LCONST feedback = ckcltag(1, offset, 0);
                    if(feedback.flag){
                        if(!flag)cout<<opc<<" 5 "<<feedback.constin;
                        cin>>type>>strtemp>>offset;
                        ckcltag(2, offset, feedback.constin);
                        crtagtemp.constin = feedback.constin;
                        crtagtemp.rnum = line;
                        crtagtemp.flag = 1;
                        putcrtag(crtagtemp);
                    }
                    else {
                        if(!flag)cout<<opc<<" 6 "<<strtemp<<' '<<offset;
                        cin>>type>>strtemp>>offset;
                        ckcltag(3, offset, 0);
                    }
                    if(!flag)cout<<" 6 "<<strtemp<<' '<<offset<<endl;
                }
            }
        }
        else {
            char flagtemp;
            int var1, result, base1;
            cin>>type;
            newcrtag(type, line);
            flagtemp = crtagtemp.flag;
            addrflagt = crtagtemp.addrflag;
            var1 = crtagtemp.constin;
            base1 = crtagtemp.base;
            //cout<<"//"<<crtagtemp.constin;
            if(!flag){
                cout<<opc<<' '<<type<<' ';
                if(type == 6) cout<<strtemp<<' '<<offset;
                else cout<<glbnum;
            }
            cin>>type;
            newcrtag(type, line);
            addrflagt += crtagtemp.addrflag;
            flagtemp += crtagtemp.flag;
            if(!flag) {
                cout<<' '<<type<<' ';
                if(type == 6) cout<<strtemp<<' '<<offset<<endl;
                else cout<<glbnum<<endl;
            }
            if(flagtemp == 2){
                switch(opc){
                case add: result = var1 + crtagtemp.constin; break;
                case sub: result = var1 - crtagtemp.constin; break;
                case mul: result = var1 * crtagtemp.constin; break;
                case _div: result = var1 / crtagtemp.constin; break;
                case mod: result = var1 % crtagtemp.constin; break;
                case cmpeq: result = (var1 == crtagtemp.constin); break;
                case cmplt: result = (var1 < crtagtemp.constin); break;
                case cmple: result = (var1 <= crtagtemp.constin); break;
                }
                if(crtagtemp.flag) crtagtemp.base += base1;
                else crtagtemp.base = base1;
                crtagtemp.constin = result;
                crtagtemp.rnum = line;
                crtagtemp.flag = 1;
                if(addrflagt == 1)crtagtemp.addrflag = true;
                else crtagtemp.addrflag = false;
            }
            else if(flagtemp == 1 && addrflagt){
                if(crtagtemp.flag) crtagtemp.base += base1;
                else crtagtemp.base = base1;
                crtagtemp.constin = 0;
                crtagtemp.rnum = line;
                crtagtemp.flag = 1;
                crtagtemp.addrflag = true;
            }
            else {
                crtagtemp.flag = 0;
                crtagtemp.rnum = line;
            }
            putcrtag(crtagtemp);
        }
    }
}
// add sub mul _div mod cmpeq cmple cmplt _move

void get2(int flag, int opc, int line){
    if(flag == 2){
        cin>>type;
        ckvar(type,line);
    }
    else if(flag < 2){
        if(opc == neg){
            cin>>type;
            if(!flag)cout<<opc<<' ';
            newcrtag(type, line);
            if(crtagtemp.flag){
                crtagtemp.constin = -crtagtemp.constin;
                crtagtemp.rnum = line;
                crtagtemp.flag = 1;
                putcrtag(crtagtemp);
            }
            if(!flag){
                cout<<type<<' ';
                if(type == 6) cout<<strtemp<<' '<<offset<<endl;
                else cout<<glbnum<<endl;
            }
        }
        if(opc == load){
            cin>>type;
            newcrtag(type,line);
            if(crtagtemp.addrflag && crtagtemp.flag){
                cltagtemp = ckcltag(1, crtagtemp.constin, 0);
                if(cltagtemp.flag) {
                    crtagtemp.constin = cltagtemp.constin;
                    crtagtemp.flag = 1;
                }
                else {
                    crtagtemp.flag = 0;
                    crtagtemp.constin = 0;
                }
                crtagtemp.addrflag = 0;
                crtagtemp.base = 0;
                crtagtemp.rnum = line;
                putcrtag(crtagtemp);
            }
            if(!flag)cout<<opc<<" 1 "<<glbnum<<endl;
        }
    }
}
// neg load //

void get3(int flag, int opc, int line){
    int num;
    if(flag == 2){
        if(opc == ret){
            cin>>type>>num;
        }
        else if(opc == br){
            cin>>type>>num;
            loopto[line] = num;
            loopfrom[num]=line;
            looptype[line]=1;
        }
        else if(opc != read){
            cin>>type;
            ckvar(type, line);
            if(opc == call) looptype[line]=3;
        }
    }
    else if(flag == 1)
        gets(trash);
    else if(!flag){
        if(opc == write || opc == param){
            cin>>type;
            cout<<opc<<' ';
            newcrtag(type, line);
            if(crtagtemp.flag)
                cout<<"5 "<<crtagtemp.constin<<endl;
            else {
                cout<<type<<' ';
                if(type == 6) cout<<strtemp<<' '<<offset<<endl;
                else cout<<glbnum<<endl;
            }
        }
        else {
            gets(trash);
            printf("%d%s\n", opc, trash);
        }
    }
}
// call read write br param ret //

void get4(int flag, int opc, int line){
    int inst, num;
    if(flag == 2){
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
    else if(flag < 2){
        if(opc != store && !flag){
            cin>>type;
            newcrtag(type, line);
            cin>>strtemp>>inst;
            if(crtagtemp.flag){
                if(crtagtemp.constin){
                    if(opc == blbs) cout<<blb<<" 5 1 2 "<<inst<<endl;
                    else cout<<blb<<" 5 0 2 "<<inst<<endl;
                }
                else if(!crtagtemp.constin){
                    if(opc == blbc) cout<<blb<<" 5 1 2 "<<inst<<endl;
                    else cout<<blb<<" 5 0 2 "<<inst<<endl;
                }
            }
            else cout<<opc<<" 1 "<<glbnum<<" 2 "<<inst<<endl;
        }
        else if(opc != store && flag) gets(trash);
        else {
            cin>>type;
            if(!flag)cout<<opc;
            newcrtag(type, line);
            if(crtagtemp.flag){
                num = crtagtemp.constin;
                if(!flag)cout<<" 5 "<<num;
                cin>>type;
                newcrtag(type, line);
                if(!flag)cout<<" 1 "<<glbnum<<endl;
                if(crtagtemp.flag && crtagtemp.constin){
                    if(flag == 1)addkill(crtagtemp.constin);
                    ckcltag(2, crtagtemp.constin, num);
                }
                else if(crtagtemp.flag && !crtagtemp.constin){
                    for(int i=0;i<(findsize(crtagtemp.base)- 1)*8;i+=8){
                        if(flag == 1)addkill(i+crtagtemp.base);
                        ckcltag(3, i+crtagtemp.base, 0);
                    }
                }
            }
            else {
                if(!flag){
                    if(type == 6) cout<<" 6 "<<strtemp<<' '<<offset;
                    else cout<<' '<<type<<' '<<glbnum;
                }
                cin>>type;
                newcrtag(type, line);
                if(!flag)cout<<" 1 "<<glbnum<<endl;
                if(crtagtemp.flag) {
                    if(crtagtemp.constin){
                        if(flag == 1)addkill(crtagtemp.constin);
                        ckcltag(3, crtagtemp.constin, 0);
                    }
                    else {
                        for(int i=0;i<(findsize(crtagtemp.base) - 1)*8;i+=8){
                            if(flag)addkill(i+crtagtemp.base);
                            ckcltag(3, i+crtagtemp.base, 0);
                        }
                    }
                }
            }
        }
    }
}
//blbc blbs blb store //

void get5(int flag, int opc, int line){
    if(!flag) cout<<opc<<endl;
}
// enterypc nop wrl //

void get6(int flag, int opc, int line){
    int lvbyte;
    if(flag == 2) ftag[funcnow++].inst = line;
    cin>>type>>lvbyte;
    if(!flag) cout<<opc<<' '<<type<<' '<<lvbyte<<endl;
}
// enter //

void printcltag(int a){
    int i, j, k;
    for(i=0;i<functotal;i++){
        for(j=0;j<ftag[i].bnum;j++){
            cout<<"Func:"<<i<<" Block:"<<j<<endl;
            for(k=0;k<localsize+globalsize;k++){
                if(a==1 && btag[i][j].outcltag[k].flag){
                    cout<<k<<':'<<"offset-"<<k<<" const-";
                    cout<<btag[i][j].outcltag[k].constin<<endl;
                }
                else if(a==0 && btag[i][j].incltag[k].flag){
                    cout<<k<<':'<<"offset-"<<k<<" const-";
                    cout<<btag[i][j].incltag[k].constin<<endl;
                }
            }
        }
    }
}
// debug用函数

int main(){
    int lines,opc,i;
    int temp, nextfunc;
    errno = 75;
    temp = 3;
    while(temp--){
        if(temp < 2){
            if(temp == 1){
            functotal = funcnow;
            getcfg(lines);
            updatesize();
            }
            if(!temp){
                //cout<<"<========================>"<<endl;
                //printcltag(1);
                //cout<<"<========================>"<<endl;
                mergeblockin();
                //cout<<"<========================>"<<endl;
                //printcltag(0);
                //cout<<"<========================>"<<endl;
            }
            funcnow = 0;
            blocknow = 0;
            if(functotal == 1)nextfunc =lines;
            else nextfunc = ftag[1].inst;
            memset(tempcltag, 0, sizeof(tempcltag));
        }
        //if(temp < 2)cout<<"<===<===<===<===<>===>===>===>===>"<<endl;
        cin>>lines;
        if(!temp) cout<<lines<<endl;
        for(i=2; i<lines; i++){
            if(temp < 2){
                if(i >= btag[funcnow][blocknow]._end){
                    if(temp){
                        memcpy(btag[funcnow][blocknow].gencltag, tempcltag, sizeof(tempcltag));
                        memcpy(btag[funcnow][blocknow].outcltag, tempcltag, sizeof(tempcltag));
                    }
                    //cout<<"==============================="<<endl;
                    memset(tempcltag, 0, sizeof(tempcltag));
                    memset(crtag, 0, sizeof(crtag));
                    blocknow++;
                    if(i >= nextfunc){
                        funcnow++;
                        blocknow = 0;
                        if(funcnow == functotal - 1) nextfunc = lines;
                        else nextfunc = ftag[funcnow+1].inst;
                    }
                    if(!temp) memcpy(tempcltag, btag[funcnow][blocknow].incltag, sizeof(tempcltag));
                    //cout<<"===>"<<funcnow<<':'<<blocknow<<"<==="<<endl;
                }
            }
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

            case blbc: case blbs: case store:
                get4(temp, opc, i);
                break;

            case wrl: case entrypc: case nop:
                get5(temp,opc, i);
                break;
            case enter:
                get6(temp, opc, i);
                break;
            }
            if(temp < 2)freecrtag(i);
            //if(temp == 1)cout<<"Now "<<opc<<"-"<<i<<endl;
        }
        cin>>lines;
        if(!temp)cout<<lines<<endl;
    }
    return 0;
}

