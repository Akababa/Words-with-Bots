#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "move.h"
using namespace std;

bool debug=false;

unordered_set<string> words;
char board[13][13];
int adj[2][13][13]; //scores of words adjacent to a square
bool legal[2][26][13][13];
int wordmult[13][13];
int letrmult[13][13];

vector<char> bag;
vector<Move> moves;
vector<char> rack;
//vector<string> letters[26]; 
int val[27];
int num[26];
int scoreword(int i,int j,bool across,bool nonadj,bool illegal);

void calcadj(){
    for(int i=1;i<=11;i++)
        for(int j=1;j<=11;j++)
            adj[1][i][j]=adj[0][i][j]=0;

    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++){
                    for(int jj=-1;jj<=1;jj++){
                        if((ii+jj)%2 && board[i+ii][j+jj]>1) {
                            board[i][j]='a'+26; // dummy character to discount ij
                            adj[ii*ii][i][j]=scoreword(i,j,jj!=0,true,true);
                            board[i][j]=0;
                        }
                    }
                }
            }
        }
    }
}

bool isWord(string& s){
    return words.count(s);
}

void docommand(string s);

// Find the score of one word (across or down) at i,j or -1 if illegal
// nonadj=true checks non-adjacent positions
// illegal= true finds score of illegal words
string retword; //last checked word for convenience
int scoreword(int i,int j,bool across,bool nonadj=false,bool illegal=false){
    int score=0;
    if(across){
        if(nonadj || adj[0][i][j]){ //check horiz
            int j1=j; while(board[i][--j1]>1);
            int j2=j; while(board[i][++j2]>1);
            /*if(j2-j1==2){
                retword=string(1,board[i][j]);
                return val[board[i][j]-'a'];
            }*/

            retword=string(j2-j1-1,0);
            for(int jj=j1+1;jj<j2;jj++) {
                retword[jj-j1-1]=board[i][jj];
                score+=val[board[i][jj]-'a'];
                //cout<<board[i][j]-'a'<<endl;
            }
            if(debug) cout <<i<<","<<j1<<"-"<<j2<<": "<<retword<<" "<<isWord(retword)<<endl;
            if(illegal) return score;
            if(!isWord(retword)) return -1;
            else return score;
        }else return 1;
    }else{
        if(nonadj || adj[1][i][j]){ //check vert
            int i1=i; while(board[--i1][j]>1);
            int i2=i; while(board[++i2][j]>1);
            /*if(i2-i1==2){
                retword=string(1,board[i][j]);
                return val[board[i][j]-'a'];
            }*/

            retword=string(i2-i1-1,0);
            for(int ii=i1+1;ii<i2;ii++){
                retword[ii-i1-1]=board[ii][j];
                score+=val[board[ii][j]-'a'];
            }
            if(debug) cout <<i1<<"-"<<i2<<","<<j<<": "<<retword<<" "<<isWord(retword)<<endl;
            if(illegal) return score;
            if(!isWord(retword)) return -1;
            else return score;
        } else return 1;
    }
    //remember to return true!!!
}

bool islegal(int i,int j){
    return scoreword(i,j,true)!=-1 && scoreword(i,j,false)!=-1;
}
//inefficient islegal just for cmd line purposes
bool islegal(const Move &m){
    bool ret=true;
    if(m.across){
        for(int j=0;j<m.length;j++)
            swap(board[m.row][m.col+j],m.word[j]);
        
        for(int j=0;j<m.length;j++){
            if(!islegal(m.row,m.col+j)){
                ret=false;break;
            }
        }
        for(int j=0;j<m.length;j++)
            swap(board[m.row][m.col+j],m.word[j]);
        
    }else{
        for(int i=0;i<m.length;i++){
            swap(board[m.row+i][m.col],m.word[i]);
        }
        for(int i=0;i<m.length;i++){
            if(!islegal(m.row+i,m.col)){
                ret=false;break;
            }
        }
        for(int i=0;i<m.length;i++){
            swap(board[m.row+i][m.col],m.word[i]);
        }
    }
    return ret;
}

// DONE: Non-contiguous moves
// accadj accumulates the adjacent word scores
// len is distance from the first
void findmovesat(int i,int j,bool acr,int len=0,int accadj=0){
    if(board[i][j]==1) return; // outside board

    if(board[i][j]) // on an occupied square
        return findmovesat(i+!acr,j+acr,len+1,acr,accadj);
    
    // on an empty square
    for(int idc=0;idc<rack.size();idc++){
        char c=rack[idc];
        if(!legal[acr][c-'a'][i][j]) continue; //adjacent word illegal, abort
        board[i][j]=c;
        if(accadj || adj[0][i][j] || adj[1][i][j]){
            int sc=scoreword(i,j,acr,true);
            if(sc!=-1)
                moves.push_back(Move(retword,retword.size(),i-len*!acr,j-len*acr,acr,accadj+sc));
        }
        if(rack.size()>1){
            rack[idc]=rack.back();
            rack.pop_back();
            findmovesat(i+!acr,j+acr,acr,len+1,accadj + adj[acr][i][j]);
            rack.push_back(rack[idc]);
            rack[idc]=c;
        }
        board[i][j]=0;
    }
}

void findmoves(){
    moves.clear();
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0) {
                findmovesat(i,j,true);
                findmovesat(i,j,false);
            }
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
        char ch;int number,value;fs>>ch>>number>>value;
        num[ch-'A']=number;
        val[ch-'A']=value;
    }
    //for(int i:val) cout <<i <<" "<<endl;
    fs.close();
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
    ios init(NULL);
    init.copyfmt(cout);
    cout<<s<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<setfill(' ')<<setw(3)<<arr[i][j];
        }
        cout<<endl;
    }
    cout.copyfmt(init);
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
                legal[0][c-'a'][i][j]=scoreword(i,j,true)!=-1;
                legal[1][c-'a'][i][j]=scoreword(i,j,false)!=-1;
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
        if(line) line>>acr; else acr='d';
        placemove(Move(ss.c_str(),ss.size(),i,j,acr[0]!='d'));
        calcadj();
    }else if(com=="il"){
        int i,j;string ss,acr;       
        if(!(line>>ss>>i>>j)) return;
        if(line) line>>acr; else acr='d';
        Move m(ss.c_str(),ss.size(),i,j,acr[0]!='d');
        cout<<m<<" legal: "<<islegal(m)<<endl;
    }else if(com=="rm"){
        int i,j;
        if(!(line>>i>>j)) return;
        board[i][j]=0;
        calcadj();
    }else if(com=="pr"){
        string ss;line>>ss;
        for(char cc:ss){
            switch(cc){
                case 'b': printboard();break;
                case 'a': 
                    print(adj[0],"adj[0]:");
                    print(adj[1],"adj[1]:");
                    break;
                case 'r':
                    cout << "rack: ";
                    for(char c:rack) cout<<c;
                    cout<<endl;
                    break;
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
        sort(moves.rbegin(),moves.rend());
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
    calcadj();
    while(cin){
        string s;getline(cin,s);
        docommand(s);
    }
}
