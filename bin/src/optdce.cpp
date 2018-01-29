#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <queue>
#include <errno.h>
using namespace std;
#define globalsize 512
// 定义最大全局变量等效long数量
#define localsize 512
// 最大的函数局部变量等效long数量
#define paramsize 16
// 最大的单个函数参量数目
#define maxbfrom 16
// 表示一个基本块最多有多少个入口
#define funcsize 32
// 程序支持的最大函数数量（包含主函数）
#define instr_max_size 32768
// 程序设计支持的最大三地址指令条数
#define np_maxsize 64
// def队列的不准确引用的最大连续数目
#define blocksize 128
// 这里设置的是单个函数中最多的基本块数量
#define maxadd 32768
// 这里设置的是CSC存储结构中最高GP偏移地址
#define max_super_children 2048
// 这里设置的是作为超级节点最大的子节点数目
#define var_maxsize localsize+paramsize+globalsize
// 确定程序允许的最大函数参量个数

enum {add = 1, sub, mul, _div, mod, neg, cmpeq, cmple, cmplt,\
      br, blb, blbc, blbs, call, load, store, _move, read, write, wrl,\
      param, enter, entrypc, ret, nop};

struct TREENODE{
    int use[np_maxsize];
    char marked;
    char type;
    int vartag;
    int basevartag;
}tnode[instr_max_size];
// 依赖树的三地址指令粒度结点

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

struct FUNC{
    int bnum;
    int lnum;
    int inst;
}ftag[funcsize];
// 这里存放了所有的函数相关数据

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
    char out[var_maxsize];
    // 输出的常量传播记录
    char in[var_maxsize];
    // 输入的常量传播记录
    char kill[var_maxsize];
    // 确定所有基本块内的kill集合
    char gen[var_maxsize];
    // 确定块内产生的活跃性集合
    int defqueue[var_maxsize][np_maxsize];
    // 存放当前变量定义的最后位置
}btag[funcsize][blocksize];
// 这里设置的是基本块的数据，包含跳转位置
// 每个基本块都保留了一个开始位置以及基本块
// 并记录在基本块开始和结尾的活跃信息

int supernode[max_super_children+var_maxsize];
// 以及每个基本块的超级汇点集合
char tosuper[instr_max_size];
// 表示对应位置的指令是否是一个活跃无关指令
char pushed[instr_max_size];
// 一个标记用的数组，加速BFS的处理过程
int loopto[instr_max_size];
int loopfrom[instr_max_size];
int looptype[instr_max_size];
// 同时也包含了分支跳转信息
// loopto表示要跳转到哪里
// loopfrom表示要从哪跳过来
// looptype表示跳转类型（条件/非条件）

char flag_main=0, flag_loop=0, flagin[var_maxsize];
int type, offset, blocknow, glbnum, supertotal = 0;
int funcnow=0, functotal, usenow = 0, funcmain, maxgtag;
string strtemp, name, strtempin, errorinfo;
TREENODE tnodetemp1, tnodetemp2;
char instr[100], livetemp[var_maxsize];
queue<int> bfsqueue;

bool cmpl(LOCAL a, LOCAL b){
    return a.offset < b.offset;
}
bool cmpg(GLOBAL a, GLOBAL b){
    return a.offset > b.offset;
}

int tagnum(int offset){
    if(offset > maxadd - (globalsize << 4))
        return ((maxadd-offset)>>3)+localsize;
    else if(offset < 0) return (-offset)>>3;
    else if(offset < (paramsize << 3) + 8)
        return (offset>>3) + localsize + globalsize - 1;
    else if(offset > 0){
        errorinfo = "Fatal Error: Parameter <paramsize> Not Big Enough!";
        perror(errorinfo.c_str());
        exit(0);
    }
}
// 这里处理的是从offset到标号的映射处理

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
        if(gtag[j].offset == 0){
            maxgtag = tagnum(gtag[j-1].offset);
            break;
        }
        else if(j == 0) {
            gtag[j]._size = (maxadd - gtag[j].offset)>>3;
        }
        else gtag[j]._size = (gtag[j-1].offset - gtag[j].offset)>>3;
    }
}
// 对所有变量容器进行排序，使我们可以根据offset快速对应到变量
// 同时更新所有变量的长度信息来区分数组，方便及进行数据定义声明

void newgvar(){
    if(offset < maxadd - (globalsize << 3)){
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
    if(offset > (paramsize << 4)){
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
                int nextbnum, callf;
                nextb = btag[i][j+1].start;
                btag[i][j]._end = nextb;
                //cout<<i<<':'<<j<<"->"<<btag[i][j]._end<<endl;
                switch(looptype[nextb-1]){
                case 0:
                    btag[i][j].bto[0] = j+1;
                    btag[i][j].bto[1] = -1;
                    btag[i][j+1].bfrom[btag[i][j+1].bfromnow++] = j;
                    break;
                case 3:
                    callf = loopto[nextb-1];
                    btag[i][j].bto[0] = j+1;
                    btag[i][j].bto[1] = -2-callf;
                    btag[callf][0].bfrom[btag[callf][0].bfromnow++] = -(i + j << 3);
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
                if(i == funcmain) btag[i][j].bto[0] = -1;
                else btag[i][j].bto[0] = -2 - funcsize + 1;
                btag[i][j].bto[1] = -1;
                if(i+1 != functotal)btag[i][j]._end = ftag[i+1].inst;
                else btag[i][j]._end = lines;
                //cout<<i<<':'<<j<<"->"<<btag[i][j]._end<<endl;
            }
        }
    }
}
// 生成程序的CFG相关信息并存储起来

void printit(int f, int b){
    cout<<"========================"<<endl;
    cout<<"Func "<<f<<" Block "<<b<<endl;
    cout<<btag[f][b].start<<" TO "<<btag[f][b]._end-1<<endl;
    cout<<"IN:";
    for(int i=0;i<var_maxsize;i++)
        if(btag[f][b].in[i]) cout<<' '<<i;
    cout<<endl;
    cout<<"OUT:";
    for(int i=0;i<var_maxsize;i++)
        if(btag[f][b].out[i]) cout<<' '<<i;
    cout<<endl;
    cout<<"GEN:";
    for(int i=0;i<var_maxsize;i++)
        if(btag[f][b].gen[i]) cout<<' '<<i;
    cout<<endl;
    cout<<"KILL:";
    for(int i=0;i<var_maxsize;i++)
        if(btag[f][b].kill[i]) cout<<' '<<i;
    cout<<endl;
    cout<<"========================"<<endl;
}
// debug用函数

char procin(int fnum, int bnum){
    int i;
    char flagret = 0;
    for(i=0;i<var_maxsize;i++){
        if(!btag[fnum][bnum].kill[i] && btag[fnum][bnum].out[i])
            livetemp[i] = btag[fnum][bnum].out[i];
    }
    // TEMP = OUT - kill
    for(i=0;i<var_maxsize;i++){
        livetemp[i] = livetemp[i] | btag[fnum][bnum].gen[i];
        if(btag[fnum][bnum].in[i] != livetemp[i]){
            btag[fnum][bnum].in[i] = livetemp[i] | btag[fnum][bnum].in[i];
            flagret = 1;
        }
    }
    // IN = TEMP 并 GEN
    return flagret;
}
// 这里进行进行块内从in到out的变化处理

void mergeblockout(){
    int i, j, y;
    char changeflag;
    //cout<<"Mergeout "<<endl;
    for(i=0;i<functotal;i++){
        changeflag = 0;
        //cout<<"===================="<<endl;
        j=ftag[i].bnum - 1;
        if(i == funcmain) j--;
        for(;j>-1;j--){
            //cout<<"Deal with "<<i<<' '<<j<<endl;
            memset(livetemp, 0, sizeof(livetemp));
            //cout<<"Check bto "<<btag[i][j].bto[0]<<" & "<<btag[i][j].bto[1]<<endl;
            for(y=0;y<var_maxsize;y++){
                if(btag[i][j].bto[0] < -1)livetemp[y] = btag[-2 - btag[i][j].bto[0]][0].in[y];
                else if(btag[i][j].bto[1] == -1)livetemp[y] = btag[i][btag[i][j].bto[0]].in[y];
                else if(btag[i][j].bto[1] < -1)
                    livetemp[y] = btag[i][btag[i][j].bto[0]].in[y] | btag[-2 - btag[i][j].bto[1]][0].in[y];
                else livetemp[y] = btag[i][btag[i][j].bto[0]].in[y] | btag[i][btag[i][j].bto[1]].in[y];
            }
            // TEMP = 交汇所有后继
            for(y=0;y<var_maxsize;y++){
                if(livetemp[y] != btag[i][j].out[y]){
                    changeflag = 1;
                    //cout<<"Warning!"<<y<<endl;
                    btag[i][j].out[y] = livetemp[y];
                }
            }
            // OUT = TEMP
            //cout<<"After mergeout:"<<(int)changeflag<<endl;
            memset(livetemp, 0, sizeof(livetemp));
            if(procin(i, j)) changeflag ++;
            //cout<<"After procin:"<<(int)changeflag<<endl;
            //printit(i, j);
        }
        if(changeflag) i--;
    }
}
// 该函数主要被用来进行基于CFG的常量传播信息合并处理，
// 最后的结果的是每个块依据CFG的常量传播输入情况

void markdc(){
    int i, inst;
    for(i=0;i<supertotal;i++){
        if(supernode[i] && !pushed[supernode[i]]){
            bfsqueue.push(supernode[i]);
            pushed[supernode[i]]=1;
            //cout<<"Push -> "<<supernode[i]<<endl;
        }
    }
    while(!bfsqueue.empty()){
        inst = bfsqueue.front();
        bfsqueue.pop();
        tnode[inst].marked = 1;
        //cout<<"MARK====>"<<inst<<endl;
        for(i=0;tnode[inst].use[i] != 0;i++){
            if(!pushed[tnode[inst].use[i]]){
                bfsqueue.push(tnode[inst].use[i]);
                pushed[tnode[inst].use[i]]=1;
                //cout<<"Push when bfs -> "<<tnode[inst].use[i]<<endl;
            }
        }
    }
}
// 通过BFS进行非死代码的标记

void buildtree(int f, int b){
    int i, j;
    for(i=0; i<var_maxsize;i++)
        if(btag[f][b].out[i]){
            for(j=0;j<np_maxsize && btag[f][b].defqueue[i][j];j++)
                supernode[supertotal++] = btag[f][b].defqueue[i][j];
        }
    // 将out集合加入supernode中
    //cout<<btag[f][b].start<<" TO "<<btag[f][b]._end<<endl;
    for(i=btag[f][b].start;i<btag[f][b]._end;i++){
        if(tosuper[i]) supernode[supertotal++] = i;
    }
    // 将tosuper集合加入到supernode中
}
// 构造完整的依赖树并进行标记

void updatebfrom(int i, int j){
    int z;
    int btoin = btag[i][j].bto[0];
    if(btoin >= 0){
        //cout<<"Func"<<i<<" Block"<<j<<" "<<btoin<<endl;
        for(z=0;z<btag[i][btoin].bfromnow;z++){
            if(btag[i][btoin].bfrom[z] == j){
                btag[i][btoin].bfrom[z] = -1;
                btag[i][btoin].bfromnow--;
                break;
            }
        }
        // 消除第一个函数内到达基本块的前继信息
        btoin = btag[i][j].bto[1];
        if(btoin){
            for(z=0;z<btag[i][btoin].bfromnow;z++){
                if(btag[i][btoin].bfrom[z] == j){
                    btag[i][btoin].bfrom[z] = -1;
                    btag[i][btoin].bfromnow--;
                    break;
                }
            }
        }
        // 消除第二个函数内到达基本块的前继信息
        else if(btoin < -1){
            btoin = (-btoin - 2);
            //cout<<"Decline Func Callers "<<btoin<<endl;
            for(z=0;z<btag[btoin][0].bfromnow;z++){
                if(btag[btoin][0].bfrom[z] == -(i + (j<<3))){
                    btag[btoin][0].bfrom[z] = -1;
                    btag[btoin][0].bfromnow--;
                    break;
                }
            }
        }
    }
    // 如果bto[0]都是负值，要么是-1，不做任何处理
    // 要么后继是保守信息存放块，无需处理
}
// 更新基本块的前继信息

void markinst(int flag){
    int i, j, k, z;
    for(i=0;i<functotal;i++){
        for(j=0;j<ftag[i].bnum;j++){
            if(flag){
                if((btag[i][j].bfromnow == 0 && j != 0) || \
                   (i != funcmain && btag[i][0].bfromnow == 0)){
                    //cout<<btag[i][j].start<<" TO "<<btag[i][j]._end-1<<endl;
                    //cout<<"Kill Block"<<endl;
                    //if(btag[i][0].bfromnow == 0)cout<<"Func Not Avaliable"<<endl;
                    for(k=btag[i][j].start;k<btag[i][j]._end;k++)
                        tnode[k].marked = 0;
                }
                else {
                    memset(supernode, 0, sizeof(supernode));
                    supertotal = 0;
                    buildtree(i, j);
                    markdc();
                }
                if(i != funcmain && btag[i][0].bfromnow == 0){
                    tnode[ftag[i].inst].marked=0;
                    if(i+1 != funcmain)tnode[btag[i][ftag[i].bnum-1]._end - 1].marked = 0;
                    else tnode[btag[i][ftag[i].bnum-1]._end - 2].marked = 0;
                }
            }
            else if(btag[i][j].bfromnow == 1){
                for(z=0;z<maxbfrom;z++)
                    if(btag[i][j].bfrom[z] > j){
                        btag[i][j].bfromnow = 0;
                        updatebfrom(i, j);
                    }
            }
            // 处理循环头的基本块
            else if(btag[i][j].bfromnow == 0 && (j != 0 || i != funcmain))
                updatebfrom(i, j);
        }
    }
}
// 根据cfg进行基本块粒度的死代码标记
// 即如果一个基本块没有输入那么全部为死代码
// flag = 0 的时候表示进行可达关系的关系传递
// flag = 1 的时候表示进行真正的消除

TREENODE ckvar(int use, int flag, int typein, int line){
    TREENODE tnodetemp;
    if(typein == 1) {
        cin>>glbnum;
        if(flag == 1){
            tnodetemp = tnode[glbnum];
            if(use)tnode[line].use[usenow++] = glbnum;
        }
    }
    else if(typein == 6){
        cin>>name>>offset;
        int strl = name.size();
        if(flag == 1){
            if(strl > 5 && name.substr(strl-5, 5) == "_base"){
                tnodetemp.basevartag = offset;
                tnodetemp.vartag = offset;
                tnodetemp.type = 0;
            }
            else if(strl > 7 && name.substr(strl-7, 7) == "_offset"){
                tnodetemp.type = 7;
                tnodetemp.basevartag = offset;
            }
            else{
                tnodetemp.type = 3;
                tnodetemp.basevartag = offset;
                if(use && btag[funcnow][blocknow].defqueue[tagnum(offset)][0])
                    tnode[line].use[usenow++] = btag[funcnow][blocknow].defqueue[tagnum(offset)][0];
                else if(btag[funcnow][blocknow].in[tagnum(offset)] == 0 && \
                        btag[funcnow][blocknow].kill[tagnum(offset)] == 0)
                    btag[funcnow][blocknow].in[tagnum(offset)] = 1;
            }
        }
        else{
            if(offset < (paramsize << 4)){
                if(strl > 5 && name.substr(strl-5, 5) == "_base")
                    newlvar(1);
                else if(!(strl > 7 && name.substr(strl-7, 7) == "_offset"))
                    newlvar(2);
            }
            else newgvar();
        }
    }
    else if(typein != 2){
        cin>>glbnum;
        tnodetemp.type = 7;
        if(typein == 5) tnodetemp.basevartag = glbnum;
        else tnodetemp.basevartag = 0;
    }
    else cin>>glbnum;
    return tnodetemp;
}
// 用于虚拟寄存器的活跃性分析和参数、局部变量的信息记录

// 下面是扫描过程中的的处理函数
void get1(int flag, int opc, int line){
    cin>>type;
    tnodetemp1 = ckvar(1, flag, type, line);
    if(flag == 2){
        cin>>type;
        tnodetemp2 = ckvar(1, flag, type, line);
    }
    else if(flag == 1){
        cin>>type;
        if(opc == _move){
            tnodetemp2 = ckvar(0, flag, type, line);
            btag[funcnow][blocknow].kill[tagnum(tnodetemp2.basevartag)] = 1;
            btag[funcnow][blocknow].defqueue[tagnum(tnodetemp2.basevartag)][0] = line;
        }
        else {
            tnodetemp2 = ckvar(1, flag, type, line);
            int types = tnodetemp1.type + tnodetemp2.type;
            //cout<<"Types Now ==="<<types<<endl;
            if(types > 17) tnode[line].type = 15;
            // have notgood
            else {
                switch(types){
                case 6: // lvar op lvar
                case 10: // lvar op const
                    tnode[line].type = 15;
                    break;
                case 7: // precise op const
                    tnode[line].type = 0;
                    if(!tnodetemp1.type) {
                        tnode[line].vartag = tnodetemp1.vartag + tnodetemp2.basevartag;
                        tnode[line].basevartag = tnodetemp1.basevartag;
                    }
                    else {
                        tnode[line].vartag = tnodetemp2.vartag + tnodetemp1.basevartag;
                        tnode[line].basevartag = tnodetemp2.basevartag;
                    }
                    break;
                case 14: // const op const
                    tnode[line].type = 7;
                    switch(opc){
                    case add: tnode[line].basevartag = tnodetemp1.basevartag + tnodetemp2.basevartag;break;
                    case sub: tnode[line].basevartag = tnodetemp1.basevartag - tnodetemp2.basevartag;break;
                    case mul: tnode[line].basevartag = tnodetemp1.basevartag * tnodetemp2.basevartag;break;
                    }
                    break;
                case 9: // nprecise op const
                case 15: // notgood op precise
                case 17: // notgood op notprecise
                    tnode[line].type = 2;
                    if(tnodetemp1.type < 3)
                        tnode[line].basevartag = tnodetemp1.basevartag;
                    else tnode[line].basevartag = tnodetemp2.basevartag;
                    break;
                }
            }
        }
    }
}
// add sub mul _div mod cmpeq cmple cmplt _move

void get2(int flag, int opc, int line){
    cin>>type;
    tnodetemp1 = ckvar(1, flag, type,line);
    if(flag == 1 && opc == load){
        if(tnodetemp1.type == 0){
            //cout<<"Find Def "<<tagnum(tnodetemp1.vartag)<<endl;
            if(btag[funcnow][blocknow].in[tagnum(tnodetemp1.vartag)] == 0 &&\
                btag[funcnow][blocknow].kill[tagnum(tnodetemp1.vartag)] == 0)
                btag[funcnow][blocknow].in[tagnum(tnodetemp1.vartag)] = 1;
            for(int i=0;i<np_maxsize;i++){
                if(btag[funcnow][blocknow].defqueue[tagnum(tnodetemp1.vartag)][i] != 0)
                    tnode[line].use[usenow++] = btag[funcnow][blocknow].defqueue[tagnum(tnodetemp1.vartag)][i];
                else break;
            }
        }
        else {
            // cout<<"Types Now ===2"<<endl;
            // cout<<"Size "<<findsize(tnodetemp1.basevartag)<<endl;
            for(int i = tagnum(tnodetemp1.basevartag);i>tagnum(tnodetemp1.basevartag) \
                - findsize(tnodetemp1.basevartag);i--){
                for(int j=0;j<np_maxsize;j++){
                    if(btag[funcnow][blocknow].defqueue[i][j] != 0)
                        tnode[line].use[usenow++] = btag[funcnow][blocknow].defqueue[i][j];
                    else break;
                }
                if(btag[funcnow][blocknow].in[i] == 0 && btag[funcnow][blocknow].kill[i] == 0)
                    btag[funcnow][blocknow].in[i] = 1;
            }
        }
        tnode[line].type = 15;
    }
}
// neg load //

void get3(int flag, int opc, int line){
    int num, num1;
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
        else if(opc == blb){
            cin>>type>>num1;
            cin>>type>>num;
            if(num1){
                loopto[line] = num;
                loopfrom[num] = line;
            }
            else {
                loopto[line] = line+1;
                loopfrom[line+1] = line;
            }
            looptype[line]=1;
        }
        else if(opc != read){
            cin>>type;
            ckvar(1, flag, type, line);
            if(opc == call) {
                looptype[line]=3;
                for(int i=0;i < funcnow+1;i++)
                    if(ftag[i].inst == glbnum){
                        loopto[line] = i;
                        //cout<<"Call Func_"<<i<<endl;
                        break;
                    }
            }
        }
    }
    else if(flag == 1){
        switch(opc){
        case blb:
            cin>>type>>num>>type>>num;
            tnode[line].marked = 1;
            break;
        case call: case br: case ret:
            cin>>type>>num;
            tnode[line].marked = 1;
            break;
        case param: case write:
            cin>>type;
            ckvar(1, flag, type, line);
        case read:
            tosuper[line] = 1;
            break;
        }
    }
}
// call read write br blb param ret //

void get4(int flag, int opc, int line){
    cin>>type;
    ckvar(1, flag, type, line);
    cin>>type;
    tnodetemp2 = ckvar(1, flag, type, line);
    if(opc == store){
        if(flag == 1 && tnodetemp2.type == 0){
            //cout<<"Types Now ===0  "<<tnodetemp2.vartag<<endl;
            btag[funcnow][blocknow].kill[tagnum(tnodetemp2.vartag)] = 1;
            memset(btag[funcnow][blocknow].defqueue[tagnum(tnodetemp2.vartag)], 0, np_maxsize * 4);
            btag[funcnow][blocknow].defqueue[tagnum(tnodetemp2.vartag)][0] = line;
        }
        else if(flag == 1 && tnodetemp2.type == 2){
            //cout<<"Types Now ===2"<<endl;
            for(int i = tagnum(tnodetemp2.basevartag);i>tagnum(tnodetemp2.basevartag) \
                - findsize(tnodetemp2.basevartag);i--){
                for(int j=0;j<np_maxsize;j++)
                    if(btag[funcnow][blocknow].defqueue[i][j] == 0){
                        btag[funcnow][blocknow].defqueue[i][j] = line;
                        break;
                    }
            }
        }
    }
    else {
        if(flag == 2){
            loopto[line] = glbnum;
            loopfrom[glbnum]=line;
            looptype[line]=2;
        }
        else tosuper[line] = 1;
    }
}
//blbc blbs store //

void get5(int flag, int opc, int line){
    if(flag == 2 && opc == entrypc) funcmain = funcnow;
    if(flag == 1) tnode[line].marked = 1;
}
// enterypc nop wrl //

void get6(int flag, int opc, int line){
    cin>>type>>glbnum;
    if(flag == 2) ftag[funcnow++].inst = line;
    else if(flag == 1) tnode[line].marked = 1;
}
// enter //

void printuse(int line){
    cout<<"Line: "<<line<<" use: ";
    for(int i=0;tnode[line].use[i] != 0;i++)
        cout<<' '<<tnode[line].use[i];
    cout<<endl;
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
                updatesize();
                getcfg(lines);
                funcnow = 0;
                blocknow = 0;
                if(functotal == 1) nextfunc =lines;
                else nextfunc = ftag[1].inst;
                memset(livetemp, 0, sizeof(livetemp));
            }
            else if(!temp){
                for(int i=localsize+1;i<=maxgtag;i++)
                    btag[funcsize-1][0].in[i] = 1;
                // 制作缺省的全局变量活跃信息保守设置
                // 其中btag的最后一个函数被处理为所有
                // 非主函数的默认返回位置，以获取保守的活跃性信息
                mergeblockout();
                markinst(0);
                markinst(1);
            }
        }
        cin>>lines;
        if(!temp){
            gets(instr);
            cout<<lines<<endl;
        }
        for(i=2; i<lines; i++){
            if(temp){
                if(temp == 1){
                    usenow = 0;
                    if(i >= btag[funcnow][blocknow]._end){
                        if(temp == 1) memcpy(btag[funcnow][blocknow].gen, btag[funcnow][blocknow].in, sizeof(livetemp));
                        memset(livetemp, 0, sizeof(livetemp));
                        blocknow++;
                        if(i >= nextfunc){
                            funcnow++;
                            blocknow = 0;
                            if(funcnow == functotal - 1) nextfunc = lines;
                            else nextfunc = ftag[funcnow+1].inst;
                        }
                    }
                }
                cin>>opc;
                switch(opc)
                {
                case add: case sub: case mul:
                case _div: case mod: case cmpeq:
                case cmple: case cmplt: case _move:
                    get1(temp, opc, i);
                    break;

                case neg: case load:
                    get2(temp, opc, i);
                    break;

                case call: case read: case write:
                case param: case br:
                case blb: case ret:
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
                //if(temp == 1)cout<<"Now "<<opc<<"-"<<i<<endl;
            }
            else {
                gets(instr);
                //printuse(i);
                if(tnode[i].marked)cout<<instr<<endl;
                else cout<<nop<<endl;
                //cout<<endl;
            }
        }
        cin>>lines;
        if(!temp)cout<<lines<<endl;
    }
    return 0;
}


