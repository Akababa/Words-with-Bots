#include <iostream>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <list>
#include <set>
#include "move.h"
using namespace std;

bool debug=false;

unordered_set<string> words;
char board[13][13];
int adj[2][13][13];
bool legal[2][26][13][13];

vector<Move> moves;
vector<char> rack;
//vector<string> letters[26]; 
int val[26];
int num[26];

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
void docommand(string s);
// Checks if the word formed in one direction (across or down) at i,j is legal
// checkall parameter is whether to check non-adjacent words
bool islegal(int i,int j,bool across,bool checkall=false){
    if(across){
        if(checkall || adj[0][i][j]){ //check horiz
            int j1=j; while(board[i][--j1]>1);
            int j2=j; while(board[i][++j2]>1);
            if(j2-j1==2) return true;

            string test(j2-j1-1,0);
            for(int jj=j1+1;jj<j2;jj++) test[jj-j1-1]=board[i][jj];
            if(debug) cout <<i<<","<<j1<<"-"<<j2<<": "<<test<<" "<<isWord(test)<<endl;
            return isWord(test);
        }else return true;
    }else{
        if(checkall || adj[1][i][j]){ //check vert
            int i1=i; while(board[--i1][j]>1);
            int i2=i; while(board[++i2][j]>1);
            if(i2-i1==2) return true;

            string test(i2-i1-1,0);
            for(int ii=i1+1;ii<i2;ii++) test[ii-i1-1]=board[ii][j];
            if(debug) cout <<i1<<"-"<<i2<<","<<j<<": "<<test<<" "<<isWord(test)<<endl;
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
        for(int j=0;j<m.length;j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=m.word[j];
        }
        for(int j=0;j<m.length;j++){
            if(!islegal(m.row,m.col+j)){
                ret=false;break;
            }
        }
        for(int j=0;j<m.length;j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=0;
        }
    }else{
        for(int i=0;i<m.length;i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=m.word[i];
        }
        for(int i=0;i<m.length;i++){
            if(!islegal(m.row+i,m.col)){
                ret=false;break;
            }
        }
        for(int i=0;i<m.length;i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=0;
        }
    }
    return ret;
}

string ms(char a,char b){
    return string(1,a)+b;
}
//TODO: Non-contiguous moves
void findmovesat(int i,int j,bool acr,char* wordsofar=new char[11],int len=0,bool hasadjyet=false){
    if(board[i][j]) return;//change this later

    for(int itc=0;itc<rack.size();itc++){
        char c=rack[itc];
        if(!legal[acr][c-'a'][i][j]) continue;
        board[i][j]=c;
        wordsofar[len]=c;
        if(hasadjyet || adj[1][i][j]){
            if(islegal(i,j,acr,true)){
                moves.push_back(Move(wordsofar,len+1,i-len*!acr,j-len*acr,acr));
            }
        }
        if(rack.size()>1){
            rack[itc]=rack.back();
            rack.pop_back();
            findmovesat(i+!acr,j+acr,acr,wordsofar,len+1,hasadjyet || adj[1][i][j]);
            rack.push_back(rack[itc]);
            rack[itc]=c;
        }
        board[i][j]=0;
    }
}

void findmoves(){
    moves.clear();
    /*
    char pos[13][13][7];
    int idx[13][13];
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            for(char c:rack){
                if(legal[1][c-'a'][i][j]) pos[i][j][idx[i][j]++]=c;
            }
        }
    }*/
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            findmovesat(i,j,true);
            //findmovesat(i,j,false);
        }
    }

}

void init(){
    fstream fs("wwfdict.txt");
    vector<string> all;
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
        //all.push_back(s);
    }
    fs.close();

    fs.open("letters.txt");
    while(fs){
        char ch;int number,value;cin>>ch>>number>>value;
        num[ch-'A']=number;
        val[ch-'A']=value;
    }
    /*
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

void printboard(){
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
        for(int j=0;j<m.length;j++){
            if(m.word[j]!=1)
                board[m.row][m.col+j]=m.word[j];
        }
    }else{
        for(int i=0;i<m.length;i++){
            if(m.word[i]!=1)
                board[m.row+i][m.col]=m.word[i];
        }
    }
}

void calclegal(char c){
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

void calclegalall(){
    for(char c='a';c<='z';c++) calclegal(c);
}

void docommand(string s){
    stringstream line(s);
    string com;line>>com;
    if(com=="pm"){
        int i,j;string ss,acr;
        if(!(line>>ss>>i>>j)) return;
        if(line) line>>acr; else acr='f';
        placemove(Move(ss.c_str(),ss.size(),i,j,acr[0]!='f'));
        findadj();
    }else if(com=="il"){
        int i,j;string ss,acr;       
        if(!(line>>ss>>i>>j)) return;
        if(line) line>>acr; else acr='f';
        Move m(ss.c_str(),ss.size(),i,j,acr[0]!='f');
        cout<<m<<" legal: "<<islegal(Move(ss.c_str(),ss.size(),i,j,acr[0]!='f'))<<endl;
    }else if(com=="rm"){
        int i,j;
        if(!(line>>i>>j)) return;
        board[i][j]=0;
        findadj();
    }else if(com=="pr"){
        set<string> arrs;
        while(line){
            string ss;line>>ss;
            if(ss=="b") printboard();
            if(ss=="a"){
                print(adj[0],"adj[0]:");
                print(adj[1],"adj[1]:");
            }
            if(ss=="r"){
                cout << "rack: ";
                for(char c:rack) cout<<c;
                cout<<endl;
            }
        }
        //if(arrs.size()==0) cout<<"spelinefy array to print"<<endl;
    }else if(com=="cl"){
        string ss;
        if(!(line>>ss)) return;
        calclegal(ss[0]);
        print(legal[0][ss[0]-'a'],"legal[0]["+ss+"]");
        print(legal[1][ss[0]-'a'],"legal[1]["+ss+"]");
    }else if(com=="ra"){
        while(line){
            string ss;line>>ss;
            if(ss[0]=='+') for(int i=1;i<ss.size();i++) rack.push_back(ss[i]);
            if(ss[0]=='-') for(int i=1;i<ss.size();i++) rack.erase(find(rack.begin(),rack.end(),ss[i]));
        }
    }else if(com=="lm"){
        string ss;line>>ss;
        if(ss!="nc")
            calclegalall();
        findmoves();
        for(Move &m:moves){
            cout <<m<<endl;
        }
    }else if(com=="db") line>>debug;
    else if(com=="file"){
        string name;if(!(line>>name)) return;
        fstream fs{name};
        while(fs){
            string cm;getline(fs,cm);
            docommand(cm);
        }
    }else if(com=="word"){
        string w;if(!(line>>w)) return;
        cout << isWord(w)<<endl;
    }else if(com=="clear"){
        for(int i=0;i<13;i++)
            for(int j=0;j<13;j++)
                if((i%12)*(j%12)==0) board[i][j]=1; else board[i][j]=0;
    }
}

int main(){
    init();
    findadj();
    while(cin){
        string s;getline(cin,s);
        docommand(s);
    }
}
