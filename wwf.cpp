#include <iostream>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include "move.h"
using namespace std;

unordered_set<string> words;
char board[13][13];
int adj[2][13][13];

vector<Move> moves;

vector<char> rack;

void findadj(){
    for(int i=1;i<=11;i++)
        for(int j=1;j<=11;j++)
            adj[0][i][j]=adj[1][i][j]=0;

    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++)
                    for(int jj=-1;jj<=1;jj++)
                        if((ii+jj)%2 && board[i+ii][j+jj]>1) {
                            adj[jj*jj][i][j]=1;
                        }
            }
        }
    }
}

bool isWord(string& s){
    return words.count(s);
}

//string test(13,0);
bool islegal(int i,int j){
    if(adj[0][i][j]){
        int i1=i; while(board[--i1][j]>1);
        int i2=i; while(board[++i2][j]>1);
        if(i2-i1==2) return true;

        string test(i2-i1-1,0);
        for(int ii=i1+1;ii<i2;ii++) {
            (test[ii-i1-1]=board[ii][j]);
            //cout <<ii<<board[ii][j]<<endl;
        }
        //test[i-i1-1]=c;
        cout <<i1<<"-"<<i2<<","<<j;
        cout <<": "<<test<<" "<<isWord(test)<<endl;
        if(!isWord(test)) return false;
    }
    if(adj[1][i][j]){
        int j1=j; while(board[i][--j1]>1);
        int j2=j; while(board[i][++j2]>1);
        if(j2-j1==2) return true;

        string test(j2-j1-1,0);
        for(int jj=j1+1;jj<j2;jj++) test[jj-j1-1]=board[i][jj];
        //test[j-j1-1]=c;
        cout <<i<<","<<j1<<"-"<<j2; 
        cout <<": "<<test<<" "<<isWord(test)<<endl;
        if(!isWord(test)) return false;
    }
    return true;
}

bool islegal(const Move &m){
    bool ret=true;
    if(m.across){
        for(int j=0;j<m.word.size();j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=m.word[j];
        }
        for(int j=0;j<m.word.size();j++){
            if(!islegal(m.row,m.col+j)){
                ret=false;break;
            }
        }
        for(int j=0;j<m.word.size();j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=0;
        }
    }else{
        for(int i=0;i<m.word.size();i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=m.word[i];
        }
        for(int i=0;i<m.word.size();i++){
            if(!islegal(m.row+i,m.col)){
                ret=false;break;
            }
        }
        for(int i=0;i<m.word.size();i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=0;
        }
    }
    return ret;
}

void findmoves(){
    //for(pair<int,int> sq:adj){

        //moves.push_back(Move(sq.first,sq.second,"aba",true));
        //board[sq.first][sq.second]='.';
        //cout << moves.back()<<endl;

    //}
}

void init(){
    fstream fs;
    fs.open("wwf.txt");
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
    }
    //cout << words.size()<<endl;    
    for(int i=0;i<13;i++){
        for(int j=0;j<13;j++){
            if((i%12)*(j%12)==0) board[i][j]=1;
            else board[i][j]=0;
        }
    }
}

void print(){
    cout<<"board:"<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<(board[i][j]?board[i][j]:'#');
        }
        cout<<endl;
    }
    cout<<"adj[0]:"<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<adj[0][i][j];
        }
        cout<<endl;
    }
    cout<<"adj[1]:"<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<adj[1][i][j];
        }
        cout<<endl;
    }
}

void placemove(const Move &m){
    if(m.across){
        for(int j=0;j<m.word.size();j++){
            board[m.row][m.col+j]=m.word[j];
        }
    }else{
        for(int i=0;i<m.word.size();i++){
            board[m.row+i][m.col]=m.word[i];
        }
    }
}

int main(){
    init();
    board[6][5]='s';
    placemove(Move("enon",7,7));
    placemove(Move("spite",8,3));board[8][6]=0;
    placemove(Move("teeny",6,7,false));
    findadj();
    //findmoves();
    print();
    //board[5][5]='i';
    //cout<<islegal(5,5)<<endl;;
    //cout <<"movetest 1:"<<islegal(Move(5,6,"sixteen",false))<<endl;
    //print();
    //string x="aa";
    //cout << islegal(5,6,'a',false)<<" "<<0+x[2];
    while(cin){
        string s;cin>>s;
        if(s=="pm"){
            int i,j;string s,acr;cin>>s>>i>>j>>acr;
            placemove(Move(s,i,j,acr[0]!='f'));
            findadj();
        }else if (s=="il"){
            int i,j;string s,acr;cin>>s>>i>>j>>acr;
            cout<<islegal(Move(s,i,j,acr[0]!='f'))<<endl;
        }
        print();
    }
}
