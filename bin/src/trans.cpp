#include <iostream>
#include <string>
using namespace std;

string trash;
string kw[26] {"add", "sub", "mul", "div", "mod", "neg", "cmpeq", "cmple", "cmplt", "br", "blb", "blbc", "blbs", "call", "load", "store", "move", "read", "write", "wrl", "param", "enter", "entrypc", "ret", "nop"};
enum {add = 1, sub, mul, _div, mod, neg, cmpeq, cmple, cmplt, br, blb, blbc, blbs, call, load, store, _move, read, write, wrl, param, enter, entrypc, ret, nop};

void get(string a){
    int n = a.size();
    if(a[0] == '('){
        a[0]=' ';
        a[n-1]=' ';
        cout<<'1'<<a;
    }
    else if(a[0] == '['){
        a[0]= ' ';
        a[n-1]=' ';
        cout<<'2'<<a;
    }
    else if(a[0] == 'F' && a[1] == 'P' && n == 2){
        cout<<"3 0 ";
    }
    else if(a[0] == 'G' && a[1] == 'P' && n == 2){
        cout<<"4 0 ";
    }
    else if(a[0] >= '0' && a[0] <= '9'){
        cout<<"5 "<<a<<' ';
    }
    else {
        int b=a.find('#');
        a[b]=' ';
        cout<<"6 "<<a<<' ';
    }
}

int main(){
    int lines,opc;
    string rs1,rs2;
    string kwin;
    cin>>trash>>trash>>lines;
    cout<<lines<<endl;
    for(int i=2;i<lines;i++){
        cin>>trash>>trash;
        cin>>kwin;
        for(int j=0;j<26;j++){
            if(kwin==kw[j]){
                opc=j+1;
                break;
            }
        }
        switch(opc){
        case add:
        case sub:
        case mul:
        case _div:
        case mod:
        case cmpeq:
        case cmple:
        case cmplt:
        case _move:
            cin>>rs1>>rs2;
            cout<<opc<<' ';
            get(rs1);
            get(rs2);
            cout<<endl;
            break;

        case neg:
        case load:
            cin>>rs1;
            cout<<opc<<' ';
            get(rs1);
            cout<<endl;
            break;

        case call:
        case br:
        case write:
        case param:
        case ret:
            cin>>rs1;
            cout<<opc<<' ';
            get(rs1);
            cout<<endl;
            break;

        case blbc:
        case blbs:
        case store:
            cin>>rs1>>rs2;
            cout<<opc<<' ';
            get(rs1);
            get(rs2);
            cout<<endl;
            break;

        case wrl:
        case entrypc:
        case read:
            cout<<opc<<endl;
            break;
        case enter:
            cin>>rs1;
            cout<<opc<<' ';
            get(rs1);
            cout<<endl;
            break;
        }
    }
    cout<<lines<<endl;
    return 0;
}

