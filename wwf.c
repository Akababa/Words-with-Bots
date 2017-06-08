#include <iostream>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
using namespace std;

unordered_set<string> words;
char board[13][13];
vector<pair<int,int>> adj;
vector<pair<int,int>> lineacr,linedow;

void findadj(){
    for(int i=1;i<=12;i++){
        for(int j=1;j<=12;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++) for(int jj=-1;jj<=1;jj++)
                    if((ii|jj) && board[i+ii][j+jj]>1) {adj.push_back(make_pair(i+ii,j+jj)); break;}
            }
        }
    }
}

bool isWord(string& s){
    return words.count(s);
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

int main(){
    init();
    findadj();
}
