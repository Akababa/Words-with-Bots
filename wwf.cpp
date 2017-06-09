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
bool legal[2][26][13][13];

vector<Move> moves;
vector<char> rack;
vector<string> letters[26]; 

void findadj(){
    for(int i=1;i<=11;i++)
        for(int j=1;j<=11;j++)
            adj[1][i][j]=adj[0][i][j]=0;

    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++)
                    for(int jj=-1;jj<=1;jj++)
                        if((ii+jj)%2 && board[i+ii][j+jj]>1) {
                            adj[ii*ii][i][j]=1;
                        }
            }
        }
    }
}

bool isWord(string& s){
    return words.count(s);
}

bool islegal(int i,int j,bool across){
    if(across){
        if(adj[0][i][j]){ //check horiz
            int j1=j; while(board[i][--j1]>1);
            int j2=j; while(board[i][++j2]>1);
            if(j2-j1==2) return true;

            string test(j2-j1-1,0);
            for(int jj=j1+1;jj<j2;jj++) test[jj-j1-1]=board[i][jj];
            cout <<i<<","<<j1<<"-"<<j2; 
            cout <<": "<<test<<" "<<isWord(test)<<endl;
            return isWord(test);
        }else return true;
    }else{
        if(adj[1][i][j]){ //check vert
            int i1=i; while(board[--i1][j]>1);
            int i2=i; while(board[++i2][j]>1);
            if(i2-i1==2) return true;

            string test(i2-i1-1,0);
            for(int ii=i1+1;ii<i2;ii++) {
                test[ii-i1-1]=board[ii][j];
            }
            cout <<i1<<"-"<<i2<<","<<j;
            cout <<": "<<test<<" "<<isWord(test)<<endl;
            return isWord(test);
        } else return true;
    }
    //remember to return true!!!
}

//string test(13,0);
bool islegal(int i,int j){
    return islegal(i,j,true)&&islegal(i,j,false);
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

bool contains(const char (&set)[7],char c){
    return set[0]==c || set[1]==c || set[2]==0 || set[2]==c || set[3]==0 || set[3]==c || set[4]==c || set[5]==c || set[6]==c;
}

void findmoves(){
    char pos[13][13][7];
    int idx[13][13];
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            for(char c:rack){
                if(legal[1][i][j]) pos[i][j][idx[i][j]++]=c;
            }
        }
    }
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            for(int k=0;k<idx[i][j];k++){

            }
        }
    }

}

void init(){
    fstream fs;
    fs.open("wwfdict.txt");
    vector<string> all;
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
        all.push_back(s);
    }
    fs.close();/*
    for(char c='a';c<='z';c++){
        string ss(1,c);
        cout << "Reading "<<ss<<endl;
        ifstream is("data/"+ss);
        // Determine the file length
        is.seekg(0, ios_base::end);
        std::size_t size=is.tellg();
        is.seekg(0, ios_base::beg);
        // Create a vector to store the data
        vector<int> v(size/sizeof(int));
        // Load the data
        is.read((char*) &v[0], size);
        is.close();
        for(int i:v) letters[c-'a'].push_back(all[i]);
        cout<<letters[c-'a'].size()<<endl;
    }
    for(string a:letters['a'-'a']){
        cout<<a<<endl;
    }*/
    //cout << words.size()<<endl;    
    for(int i=0;i<13;i++){
        for(int j=0;j<13;j++){
            if((i%12)*(j%12)==0) board[i][j]=1;
            else board[i][j]=0;
        }
    }
}

void print(const bool (&arr)[13][13],string s){
    cout<<s<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<arr[i][j];
        }
        cout<<endl;
    }
}

void print(const int (&arr)[13][13],string s){
    cout<<s<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<arr[i][j];
        }
        cout<<endl;
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
}

void placemove(const Move &m){
    if(m.across){
        for(int j=0;j<m.word.size();j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=m.word[j];
        }
    }else{
        for(int i=0;i<m.word.size();i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=m.word[i];
        }
    }
}

void calclegal(){
    for(char c='a';c<='z';c++){
        for(int i=1;i<=11;i++){
            for(int j=1;j<=11;j++){
                if(board[i][j]==0){
                    board[i][j]=c;
                    legal[0][c-'a'][i][j]=islegal(i,j,true);
                    legal[1][c-'a'][i][j]=islegal(i,j,false);
                    board[i][j]=0;
                }else{
                    legal[0][c-'a'][i][j]=legal[1][c-'a'][i][j]=false;
                }
            }
        }
    }
}

int main(){
    init();
    findadj();
    while(cin){
        string s;cin>>s;
        if(s=="pm"){
            int i,j;string ss,acr;cin>>ss>>i>>j>>acr;
            placemove(Move(ss,i,j,acr[0]!='f'));
            findadj();
        }else if (s=="il"){
            int i,j;string ss,acr;cin>>ss>>i>>j>>acr;
            cout<<islegal(Move(ss,i,j,acr[0]!='f'))<<endl;
        }else if (s=="rm"){
            int i,j;cin>>i>>j;
            board[i][j]=0;
            findadj();
        }else if (s=="pr"){
            string ss;cin>>ss;
            if(ss[0]=='b') print();
            if(ss[0]=='a'){
                print(adj[0],"adj[0]:");
                print(adj[1],"adj[1]:");
            }
        }else if (s=="cl"){
            string ss;cin>>ss;
            calclegal();
            print(legal[0][ss[0]-'a'],"legal[0]["+ss+"]");
            print(legal[1][ss[0]-'a'],"legal[1]["+ss+"]");
        }
    }
}
