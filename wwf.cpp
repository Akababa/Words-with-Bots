#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include "move.h"
using namespace std;

bool debug=false;
bool start=true;

unordered_set<string> words; //dictionary
char board[13][13];
int adj[2][13][13]; //scores of words in perpendicular directions
bool legal[2][27][13][13];
int wordmult[13][13];
int letrmult[13][13];

vector<char> bag;
vector<Move> moves;
char* rack=new char[999];
int racksize=0;
//vector<string> letters[26]; 
int val[27];
int num[26];

int scoreword(int i,int j,bool across,bool nonadj,bool illegal);

//calculates the linear (no multipliers) scores of the word fragments already on the board
void calcadj(){
    for(int i=1;i<=11;i++)
        for(int j=1;j<=11;j++)
            adj[0][i][j]=adj[1][i][j]=0;
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++){
                    for(int jj=-1;jj<=1;jj++){
                        if((ii+jj)%2 && board[i+ii][j+jj]>1) {
                            board[i][j]='A'+26; // dummy character to discount ij
                            adj[ii*ii][i][j]=scoreword(i,j,jj!=0,true,true);
                            board[i][j]=0;
                        }
                    }
                }
            }
        }
    }
}

bool isWord(const string& s){
    return words.count(s);
}

void docommand(string s);

// Find the score of one word (across or down) at i,j or 0 if illegal
// nonadj=true checks non-adjacent positions
// illegal= true finds score of illegal words
string retword; //last checked word for convenience
int scoreword(int i,int j,bool across,bool nonadj=false,bool illegal=false){
    int score=0;
    if(across){
        if(nonadj || adj[0][i][j]){ //check horiz
            int j1=j; while(board[i][--j1]>1);
            int j2=j; while(board[i][++j2]>1);

            retword=string(j2-j1-1,0);
            for(int jj=j1+1;jj<j2;jj++) {
                retword[jj-j1-1]=board[i][jj];
                score+=val[board[i][jj]-'A'];
            }
            if(debug) cout <<i<<","<<j1<<"-"<<j2<<": "<<retword<<" "<<isWord(retword)<<endl;
            if(illegal) return score;
            if(!isWord(retword)) return 0;
            else return score;
        }else return 1;
    }else{
        if(nonadj || adj[1][i][j]){ //check vert
            int i1=i; while(board[--i1][j]>1);
            int i2=i; while(board[++i2][j]>1);
            retword=string(i2-i1-1,0);
            for(int ii=i1+1;ii<i2;ii++){
                retword[ii-i1-1]=board[ii][j];
                score+=val[board[ii][j]-'A'];
            }
            if(debug) cout <<i1<<"-"<<i2<<","<<j<<": "<<retword<<" "<<isWord(retword)<<endl;
            if(illegal) return score;
            if(!isWord(retword)) return 0;
            else return score;
        } else return 1;
    }
    //remember to return true!!!
}

// accword is the cumulative word fragment
// accadj accumulates the adjacent word scores
// accletr accumulates the letter scores (incl. bonus)
// wmult is the cumulative word multiplier
void findmovesat(int i,int j,bool acr,string accword="",int accadj=0,int accletr=0,int wmult=1){
    if(board[i][j]){ // on a non-empty square
        if(board[i][j]==1) return;
        findmovesat(i+!acr,j+acr,acr,accword+board[i][j],accadj,accletr+val[board[i][j]-'A'],wmult); //passed letter doesn't count
        return;
    }
    const bool checkleaves=accadj || adj[0][i][j] || adj[1][i][j];
    // on an empty square
    for(int idc=racksize;idc--;) {
        if(rack[idc]-'A'==26){
            for(char c='A';c<='Z';c++){
                if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort
                if(checkleaves){ 
                    if(board[i+!acr][j+acr]>1){ // takes care of trailing letters
                        int ii=i,jj=j;
                        string extra(1,c);
                        int bonus=0; //any remaining score goes here
                        while(board[ii+=!acr][jj+=acr]>1){
                            extra.push_back(board[ii][jj]);
                            bonus+=val[board[ii][jj]-'A'];
                        }
                        if(isWord(accword+extra)){
                            moves.push_back(Move(accword+extra, i-accword.size()*!acr, j-accword.size()*acr, acr,
                                accadj+ //previous adjacent scores are unchanged
                                wordmult[i][j]* (adj[acr][i][j] + //adj from this letter
                                wmult* (accletr+bonus) ) //accumulated letters+last letter
                                ));
                        }
                    }else if (isWord(accword+c)){ // no trailing letters
                        moves.push_back(Move(accword+c, i-accword.size()*!acr, j-accword.size()*acr, acr,
                            accadj+ //previous adjacent scores are unchanged
                            wordmult[i][j]* (adj[acr][i][j] + //adj from this letter
                            wmult* accletr ) //accumulated letters+last letter
                            ));
                    }
                }
                if(racksize>1){
                    rack[idc]=rack[--racksize];
                    findmovesat(i+!acr, j+acr, acr, accword+c, 
                        accadj+ wordmult[i][j]* 
                        (adj[acr][i][j]?adj[acr][i][j] + letrmult[i][j]*val[c-'A']:0), //letter mult
                        accletr+ letrmult[i][j]*val[c-'A'],
                        wmult*wordmult[i][j]); //word multiplier
                    rack[racksize++]=rack[idc];
                    rack[idc]=c;
                }
            }
            continue;
        }
        char c=rack[idc];
        if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort
        if(checkleaves){ 
            if(board[i+!acr][j+acr]>1){ // takes care of trailing letters
                int ii=i,jj=j;
                string extra(1,c);
                int bonus=0; //any remaining score goes here
                while(board[ii+=!acr][jj+=acr]>1){
                    extra.push_back(board[ii][jj]);
                    bonus+=val[board[ii][jj]-'A'];
                }
                if(isWord(accword+extra)){
                    moves.push_back(Move(accword+extra, i-accword.size()*!acr, j-accword.size()*acr, acr,
                        accadj+ //previous adjacent scores are unchanged
                        wordmult[i][j]* ( (adj[acr][i][j]? adj[acr][i][j] + letrmult[i][j]*val[c-'A'] :0) + //adj from this letter
                        wmult* (accletr+bonus+ letrmult[i][j]*val[c-'A']) ) //accumulated letters+last letter
                        ));
                }
            }else if (isWord(accword+c)){ // no trailing letters
                moves.push_back(Move(accword+c, i-accword.size()*!acr, j-accword.size()*acr, acr,
                    accadj+ //previous adjacent scores are unchanged
                    wordmult[i][j]* ( (adj[acr][i][j]? adj[acr][i][j] + letrmult[i][j]*val[c-'A'] :0) + //adj from this letter
                    wmult* (accletr+ letrmult[i][j]*val[c-'A']) ) //accumulated letters+last letter
                    ));
                //if(accword+c=="exscind"&&j==10&&!acr)cout<<accadj<<" "<<accletr<<" "<<wmult<<" "<<wordmult[i][j]<<endl;
            }
        }
        if(racksize>1){
            rack[idc]=rack[--racksize];
            findmovesat(i+!acr, j+acr, acr, accword+c, 
                accadj+ wordmult[i][j]* //adjacent accumulator
                (adj[acr][i][j]?adj[acr][i][j] + letrmult[i][j]*val[c-'A']:0),
                accletr+ letrmult[i][j]*val[c-'A'], //letter accumulator
                wmult*wordmult[i][j]); //word multiplier
            rack[racksize++]=rack[idc];
            rack[idc]=c;
        }
    }
    //accword[len]=0;
}

void findmoves(){
    moves.clear();
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){ //only look on free squares to avoid redundancy
                int j1=j; while(board[i][--j1]>1); //go to leftmost taken square on board
                findmovesat(i,j1+1,true);
                int i1=i; while(board[--i1][j]>1);
                findmovesat(i1+1,j,false);
            }
        }
    }
}

void init(){
    fstream fs("wwfdict.txt");
    words.clear();
    vector<string> all;
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
        //all.push_back(s);
    }
    fs.close();

    fs.open("letters.txt");
    bag.clear();
    while(fs){
        char ch;int number,value;fs>>ch>>number>>value;
        num[ch-'A']=number;
        val[ch-'A']=value;
        for(int i=number;i--;) bag.push_back(ch);
    } 
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle (bag.begin(), bag.end(), default_random_engine(seed));
    //for(int i:val) cout <<i <<" "<<endl;
    fs.close();

    fs.open("board1.txt");
    int w,h;fs>>w>>h;
    for(int i=1;i<12;i++)
        for(int j=1;j<12;j++)
            letrmult[i][j]=1;
    for(int i=1;i<12;i++)
        for(int j=1;j<12;j++)
            wordmult[i][j]=1;
    
    while(fs){
        int mult;fs>>mult;
        string lw;fs>>lw;
        int n;fs>>n;
        if(lw[0]=='L'){
            for(int i=n;i--;){
                int a,b;fs>>a>>b;
                letrmult[a][b]=letrmult[a][w+1-b]=letrmult[h+1-a][b]=letrmult[h+1-a][w+1-b]=mult;
            }
        }else{
            for(int i=n;i--;){
                int a,b;fs>>a>>b;
                wordmult[a][b]=wordmult[a][w+1-b]=wordmult[h+1-a][b]=wordmult[h+1-a][w+1-b]=mult;
            }
        }
    }
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

void print(const int (&arr)[13][13],string s,int width=1){
    cout<<s<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            cout<<setfill(' ')<<setw(width)<<arr[i][j];
        }
        cout<<endl;
    }
}

void printboard(){
    cout<<"board:"<<endl;
    for(int i=0;i<=12;i++){
        for(int j=0;j<=12;j++){
            switch(board[i][j]){
                case 0: cout<<(i==6&&j==6?'+':'#');break;
                case 1: cout<<(i+j<10?i+j:0); break;
                default: cout <<board[i][j];
            }
        }
        cout<<endl;
    }
}

void placemove(const Move &m){
    if(m.across){
        for(int j=0;j<m.length;j++){
            board[m.row][m.col+j]=m.word[j];
        }
    }else{
        for(int i=0;i<m.length;i++){
            board[m.row+i][m.col]=m.word[i];
        }
    }
}

void calclegal(char c){
    for(int i=1;i<=11;i++){
        for(int j=1;j<=11;j++){
            if(board[i][j]==0){
                board[i][j]=c;
                legal[0][c-'A'][i][j]=scoreword(i,j,true);
                legal[1][c-'A'][i][j]=scoreword(i,j,false);
                board[i][j]=0;
            }else{
                legal[0][c-'A'][i][j]=legal[1][c-'A'][i][j]=false;
            }
        }
    }
}

void calclegalall(){
    for(char c='A';c<='Z';c++) calclegal(c);
}

//inefficient islegal just for cmd line purposes
// modifies m if it's legal, otherwise returns false
bool islegal(Move &m){
    calcadj();
    calclegalall();
    int i=m.row,j=m.col;
    for(int nn=0;nn<m.length;nn++,i+=!m.across,j+=m.across){
        if(board[i][j]==1) return false;
        if(board[i][j]==m.word[nn]) continue;
        if(board[i][j]>1) return false;
        if(!legal[m.across][m.word[nn]-'A'][i][j]) return false;

    }
    string acc=m.word;
    int i1=m.row,j1=m.col;
    while(board[i1-=!m.across][j1-=m.across]>1)
        acc=string(1,board[i1][j1])+acc;
    
    while(board[i][j]>1){
        acc.push_back(board[i][j]);
        i+=!m.across;j+=m.across;
    }
    if(!isWord(acc)) return false;
    
    int adjscore=0;
    int wordscore=0;
    int wmult=1;
    int nn=-1;
    do{
        i1+=!m.across;j1+=m.across;nn++;
        if(board[i1][j1]) continue;
        adjscore+=wordmult[i1][j1]*(adj[m.across][i1][j1]+letrmult[i1][j1]*val[acc[nn]-'A']);
        wordscore+=letrmult[i1][j1]*val[acc[nn]-'A'];
        wmult*=wordmult[i][j];
    }while(nn<acc.size());

    m.score=adjscore+wmult*wordscore;
    return true;
}

void docommand(string ssss){
    stringstream line(ssss);
    string com;line>>com;
    if(com=="pb"){
        findmoves();
        if(moves.empty()) {cout<<"No moves"<<endl; return;}
        sort(moves.rbegin(),moves.rend());
        Move mm=moves.front();
        placemove(mm);
        printboard();
        cout <<mm<<endl;
        docommand("ra -"+string(mm.word));
        calcadj();
        calclegalall();
    }else if(com=="pm"){
        int i,j;string ss,acr;
        if(!(line>>ss)) return;
        if(!(line>>i) || !(line>>j)) i=j=6;
        else if(!(line>>acr)) acr="a";
        for(char &c:ss) c=toupper(c);
        placemove(Move(ss,i,j,acr[0]!='d'));
        calcadj();
        calclegalall();
    }else if(com=="il"){
        int i,j;string ss,acr;       
        if(!(line>>ss>>i>>j)) return;
        if(line) line>>acr; else acr='d';
        Move m(ss.c_str(),ss.size(),i,j,acr[0]!='d');
        bool leg=islegal(m);
        cout<<m<<" legal: "<<leg<<endl;
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
                    print(adj[0],"adj[0]:",3);
                    print(adj[1],"adj[1]:",3);
                    break;
                case 'r':
                    cout << "rack: ";
                    for(int i=0;i<racksize;i++) cout<<rack[i];
                    cout<<endl;
                    break;
                case 'l': print(letrmult,"letter multipliers:"); break;
                case 'w': print(wordmult,"word multipliers:"); break;
            }
        }
    }else if(com=="cl"){
        string ss;
        if(!(line>>ss)) return;
        calclegal(ss[0]);
        print(legal[0][ss[0]-'A'],"legal[0]["+ss+"]");
        print(legal[1][ss[0]-'A'],"legal[1]["+ss+"]");
    }else if(com=="ra"){
        while(line){
            string ss;line>>ss;
            if(ss[0]=='+'){
                if(isdigit(ss[1])) for(int i=ss[1]-'0';i-- && !bag.empty();) {
                    rack[racksize++]=bag.back();
                    bag.pop_back();
                }else
                    for(int i=1;i<ss.size();i++) rack[racksize++]=toupper(ss[i]);
            }
            if(ss[0]=='-')
                for(int i=1;i<ss.size();i++) {
                    char*  t=find(rack,rack+racksize,ss[i]);
                    move(t,rack+racksize--,t-1);
                }
        }
        cout << "rack: ";
        for(int i=0;i<racksize;i++) cout<<rack[i];
        cout<<endl;
    }else if(com=="lm"){
        string ss;line>>ss;
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
