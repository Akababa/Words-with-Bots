#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <sstream>
#include <algorithm>
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include "move.h"
using namespace std;
using namespace boost;

bool debug=false;
bool start=true;

unordered_set<string> words; // hashtable dictionary
char **board;
int ***adj; //scores of words in perpendicular directions
bool ****legal;
int **wordmult;
int **letrmult;
int **points; //point values of placed tiles
dynamic_bitset<> chains;

vector<char> bag;
vector<Move> moves;
char* rack=new char[999];
int racksize=0;
int val[27];
int num[26];
int bsize=15;
unsigned int depth,mod; //for pruning. mod=27^(depth-1)
string boardfile="wwf/board.txt",tilefile="wwf/tiles.txt",dictfile="wwf/dict.txt",dictbin="wwf/dict.bin";

int scoreword(int i,int j,bool across,bool nonadj,bool illegal);

//calculates the linear (no multipliers) scores of the word fragments already on the board
void calcadj(){
    for(int i=1;i<=bsize;i++)
        for(int j=1;j<=bsize;j++)
            adj[0][i][j]=adj[1][i][j]=0;
    for(int i=1;i<=bsize;i++){
        for(int j=1;j<=bsize;j++){
            if(board[i][j]==0){
                for(int ii=-1;ii<=1;ii++){
                    for(int jj=-1;jj<=1;jj++){
                        if((ii+jj)%2 && board[i+ii][j+jj]>1) {
                            //board[i][j]='A'+26; // dummy character to discount ij
                            adj[ii*ii][i][j]=scoreword(i,j,jj!=0,true,true);
                            //board[i][j]=0;
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
                score+=points[i][jj];
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
                score+=points[ii][j];
            }
            if(debug) cout <<i1<<"-"<<i2<<","<<j<<": "<<retword<<" "<<isWord(retword)<<endl;
            if(illegal) return score;
            if(!isWord(retword)) return 0;
            else return score;
        } else return 1;
    }
    //remember to return true!!!
}

void calclegal(char c){
    for(int i=1;i<=bsize;i++){
        for(int j=1;j<=bsize;j++){
            if(board[i][j]==0){
                board[i][j]=c;
                //points[i][j]=val[c-'A']; //no need to set points if we only care about legality
                legal[0][c-'A'][i][j]=scoreword(i,j,true);
                legal[1][c-'A'][i][j]=scoreword(i,j,false);
                board[i][j]=0;
                //points[i][j]=0;
            }else{
                legal[0][c-'A'][i][j]=legal[1][c-'A'][i][j]=false;
            }
        }
    }
}

void calclegalall(){
    for(char c='A';c<='Z';c++) calclegal(c);
}
// accword is the cumulative word fragment
// accadj accumulates the adjacent word scores
// accletr accumulates the letter scores (incl. bonus)
// wmult is the cumulative word multiplier
// DONE: Blank tiles don't count for points, use a dedicated point array to solve this
// TODO: using full rack bonus
string accword;
void findmovesat(int i,int j,bool acr,int accadj=0,int accletr=0,int wmult=1,int blankpos=-1,int rhash=0){
    if(board[i][j]){ // on a non-empty square
        if(board[i][j]==1) return;
        accword.push_back(board[i][j]);
        findmovesat(i+!acr,j+acr,acr,accadj,accletr+points[i][j],wmult,blankpos,((rhash+board[i][j]-'A'+1)%mod)*27); //passed letter doesn't count
        accword.pop_back();
        return;
    }
    const bool checkleaves=accadj || adj[0][i][j] || adj[1][i][j];
    // on an empty square
    for(int idc=racksize;idc--;) {
        if(rack[idc]-'A'==26){ //this is a blank
            rack[idc]=rack[--racksize];
            for(char c='A';c<='Z';c++){
                if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort
                if(chains[rhash+c-'A'+1]==0) continue; //impossible word, abort
                if(checkleaves){ 
                    if(board[i+!acr][j+acr]>1){ // takes care of trailing letters
                        int ii=i,jj=j;
                        string extra(1,c);
                        int bonus=0; //any remaining score goes here
                        while(board[ii+=!acr][jj+=acr]>1){
                            extra.push_back(board[ii][jj]);
                            bonus+=points[ii][jj];
                        }
                        if(isWord(accword+extra)){
                            moves.push_back(Move(accword+extra, i-accword.size()*!acr, j-accword.size()*acr, acr,
                                accadj+ //previous adjacent scores are unchanged
                                wordmult[i][j]* (adj[acr][i][j] + //adj from this letter
                                wmult* (accletr+bonus) ) + //accumulated letters+last letter
                                (racksize==1)*35, //bonus 35 for empty rack
                                accword.size()));
                        }
                    }else if (isWord(accword+c)){ // no trailing letters
                        moves.push_back(Move(accword+c, i-accword.size()*!acr, j-accword.size()*acr, acr,
                            accadj+ //previous adjacent scores are unchanged
                            wordmult[i][j]* (adj[acr][i][j] + //adj from this letter
                            wmult* accletr ) + //accumulated letters+last letter
                            (racksize==1)*35,
                            accword.size()));                                            
                    }
                }
                if(racksize>1){
                    accword.push_back(c);
                    findmovesat(i+!acr, j+acr, acr, 
                        accadj+ wordmult[i][j]* 
                        adj[acr][i][j], //letter mult
                        accletr,
                        wmult*wordmult[i][j], //word multiplier
                        accword.size(),
                        ((rhash+c-'A'+1)%mod)*27);
                    accword.pop_back();
                }
            }
            rack[racksize++]=rack[idc];
            rack[idc]='A'+26;
            continue;
        }
        char c=rack[idc];
        if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort //good chunk of time spent here
        if(chains[rhash+c-'A'+1]==0) continue; //impossible word, abort
        if(checkleaves){ 
            if(board[i+!acr][j+acr]>1){ // takes care of trailing letters
                int ii=i,jj=j;
                string extra(1,c);
                int bonus=0; //any remaining score goes here
                while(board[ii+=!acr][jj+=acr]>1){
                    extra.push_back(board[ii][jj]);
                    bonus+=points[ii][jj];
                }
                if(isWord(accword+extra)){
                    moves.push_back(Move(accword+extra, i-accword.size()*!acr, j-accword.size()*acr, acr,
                        accadj+ //previous adjacent scores are unchanged
                        wordmult[i][j]* ( (adj[acr][i][j]? adj[acr][i][j] + letrmult[i][j]*val[c-'A'] :0) + //adj from this letter
                        wmult* (accletr+bonus+ letrmult[i][j]*val[c-'A']) ) + //accumulated letters+last letter
                        (racksize==1)*35,
                        blankpos));                                           
                    //if(accword+extra=="NEWSLETTERS")cout<<accadj<<" "<<accletr<<" "<<wmult<<" "<<wordmult[i][j]<<
                    //    " "<<c<<" "<<i<<","<<j<<"exra:"<<extra<<"+"<<bonus<<endl;

                }
            }else if (isWord(accword+c)){ // no trailing letters
                moves.push_back(Move(accword+c, i-accword.size()*!acr, j-accword.size()*acr, acr,
                    accadj+ //previous adjacent scores are unchanged
                    wordmult[i][j]* ( (adj[acr][i][j]? adj[acr][i][j] + letrmult[i][j]*val[c-'A'] :0) + //adj from this letter
                    wmult* (accletr+ letrmult[i][j]*val[c-'A']) ) + //accumulated letters+last letter
                    (racksize==1)*35,
                    blankpos));
                //if(accword+c=="FIZZ")cout<<accadj<<" "<<accletr<<" "<<wmult<<" "<<wordmult[i][j]<<endl;
            }
        }
        if(racksize>1){
            rack[idc]=rack[--racksize];
            accword.push_back(c);
            findmovesat(i+!acr, j+acr, acr, //wow, all the time is on this line
                accadj+ wordmult[i][j]* //adjacent accumulator
                (adj[acr][i][j]?adj[acr][i][j] + letrmult[i][j]*val[c-'A']:0),
                accletr+ letrmult[i][j]*val[c-'A'], //letter accumulator
                wmult*wordmult[i][j], //word multiplier
                blankpos,
                ((rhash+c-'A'+1)%mod)*27);
            accword.pop_back(); //new hotspot?
            rack[racksize++]=rack[idc];
            rack[idc]=c;
        }
    }
}

void findmoves(){
    calcadj();
    calclegalall();
    moves.clear();
    for(int i=1;i<=bsize;i++){
        for(int j=1;j<=bsize;j++){
            if(board[i][j]==0){ //only look on free squares to avoid redundancy
                int j1=j; while(board[i][--j1]>1); //go to leftmost taken square on board
                findmovesat(i,j1+1,true);
                int i1=i; while(board[--i1][j]>1);
                findmovesat(i1+1,j,false);
            }
        }
    }
}

void writeIndex(){
    depth=5;mod=27*27*27*27;
    fstream fs(dictfile);
    string s;
    chains=boost::dynamic_bitset<> (mod*27);
    while(fs>>s){
        for(char &c:s) c-=('A'-1); //1<=c<27
        int rhash=0;
        unsigned int mmod=mod*27;
        for (int i = 0; i < s.size(); ++i){
            rhash=(rhash*27+s[i])%mmod;
            chains[rhash]=1;
        }
    }
    fs.close();

    std::vector<dynamic_bitset<>::block_type> v(chains.num_blocks());
    to_block_range(chains, v.begin());
    ofstream out(dictbin, ios::out | ios::binary);
    out.write((char*)&depth,sizeof(int));
    int x=v.size();
    out.write((char*)&x,sizeof(int));
    out.write((char*)&v[0], x * sizeof(dynamic_bitset<>::block_type));
    out.close();
}

void init(){
    fstream fs(dictfile);
    if(!fs){
        cout<<dictfile <<" not found, reverting to default"<<endl;
        fs.open("wwf/dict.txt");
    }
    words.clear();
    vector<string> all;
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
        //all.push_back(s);
    }
    fs.close();

    fs.open(tilefile);
    if(!fs){
        cout<<tilefile <<" not found, reverting to default"<<endl;
        fs.open("wwf/tiles.txt");
    }
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

    fs.open(boardfile);
    if(!fs){
        cout<<board <<" not found, reverting to default"<<endl;
        fs.open("wwf/board.txt");
    }
    fs>>bsize;
    int bt=bsize+2;
    adj = new int**[2]; //scores of words in perpendicular directions
    adj[0] = new int*[bt]; adj[1]=new int*[bt];
    legal = new bool***[2];
    legal[0]=new bool**[27];
    legal[1]=new bool**[27];
    for(int j=0;j<27;j++){
        legal[0][j]=new bool*[bt];
        legal[1][j]=new bool*[bt];
    }

    board = new char*[bt];
    wordmult = new int*[bt];
    letrmult = new int*[bt]; 
    points = new int*[bt];
    for(int i = 0; i < bt; ++i){
        board[i] = new char[bt];
        letrmult[i]=new int[bt];
        wordmult[i] = new int[bt];
        points[i]=new int[bt];
        adj[0][i]=new int[bt];
        adj[1][i]=new int[bt];
        for(int j=0;j<27;j++){
            legal[0][j][i]=new bool[bt];
            legal[1][j][i]=new bool[bt];
        }
    }

    for(int i=1;i<=bsize;i++)
        for(int j=1;j<=bsize;j++)
            letrmult[i][j]=1;
    for(int i=1;i<=bsize;i++)
        for(int j=1;j<=bsize;j++)
            wordmult[i][j]=1;
    
    while(fs){
        int mult;fs>>mult;
        string lw;fs>>lw;
        int n;fs>>n;
        if(lw[0]=='L'){
            for(int i=n;i--;){
                int a,b;fs>>a>>b;
                letrmult[a][b]=letrmult[a][bsize+1-b]=letrmult[bsize+1-a][b]=letrmult[bsize+1-a][bsize+1-b]=mult;
            }
        }else{
            for(int i=n;i--;){
                int a,b;fs>>a>>b;
                wordmult[a][b]=wordmult[a][bsize+1-b]=wordmult[bsize+1-a][b]=wordmult[bsize+1-a][bsize+1-b]=mult;
            }
        }
    }
    fs.close();

    ifstream in(dictbin);
    if(in){
        in.read((char*)&depth,sizeof(int));
        mod=1;
        for(int i=depth;--i;mod*=27);//mod=27^depth
        int y;
        in.read((char*)&y,sizeof(int));
        std::vector<dynamic_bitset<>::block_type> u(y);
        in.read((char*)&u[0],y*sizeof(dynamic_bitset<>::block_type));
        chains=dynamic_bitset<>(u.begin(),u.end());
    }else{
        in.close();
        writeIndex();
    }

    for(int i=0;i<bt;i++){
        for(int j=0;j<bt;j++){
            if((i%(bt-1))*(j%(bt-1))==0) board[i][j]=1;
            else board[i][j]=0;
        }
    }
}

void print(bool** arr,string s){
    cout<<s<<endl;
    for(int i=1;i<=bsize;i++){
        for(int j=1;j<=bsize;j++){
            cout<<arr[i][j];
        }
        cout<<endl;
    }
}

void print(int** arr,string s,int width=1){
    cout<<s<<endl;
    for(int i=1;i<=bsize;i++){
        for(int j=1;j<=bsize;j++){
            cout<<setfill(' ')<<setw(width)<<arr[i][j];
        }
        cout<<endl;
    }
}

void printboard(){
    cout<<"board:"<<endl;
    for(int i=0;i<=bsize;i++){
        for(int j=0;j<=bsize;j++){
            switch(board[i][j]){
                case 0: cout<<(i==(bsize+1)/2&&j==(bsize+1)/2?'+':'#');break;
                case 1: cout<<((i+j)%10); break;
                default: cout <<board[i][j];
            }
        }
        cout<<endl;
    }
}

// overwrite=true overwrites already placed tiles
void placemove(const Move &m,bool overwrite=false){
    for(int i=0,j=0;i+j<m.length;i+=!m.across,j+=m.across){
            if(!overwrite && board[m.row+i][m.col+j]) continue;
            board[m.row+i][m.col+j]=m.word[i+j];
            if(i+j!=m.blankpos)
                points[m.row+i][m.col+j]=val[m.word[i+j]-'A'];
    }
}

// inefficient islegal just for cmd line purposes
// modifies m if it's legal, otherwise returns false
// Does not mutate the board (!) :D
// doesn't work with blanks, nbd for now
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
    int nn=0;
    //cout <<i1<<j1<<i<<j<<endl;
    do{
        i1+=!m.across;j1+=m.across;
        if(board[i1][j1]) {
            wordscore+=val[acc[nn]-'A'];
            continue;
        }
        adjscore+= wordmult[i1][j1] * 
        (adj[m.across][i1][j1]? adj[m.across][i1][j1] + letrmult[i1][j1] * val[acc[nn]-'A']:0);
        wordscore+= letrmult[i1][j1] * val[acc[nn]-'A']; 
        wmult*= wordmult[i1][j1];
    }while(++nn<acc.size());
    //cout <<adjscore<<wordscore<<wmult;

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
    }else if(com=="pm"){
        bool override=false;
        int i,j,p;string ss,acr;
        if(!(line>>ss)) return;
        if(ss=="-f") {
            override=true;
            if(!(line>>ss)) return;
        }
        if(!(line>>i) || !(line>>j)) i=j=(bsize+1)/2;
        if(!(line>>acr)) acr="a";
        if(!(line>>p)) p=-1;
        for(char &c:ss) c=toupper(c);
        placemove(Move(ss,i,j,acr[0]!='d',0,0,p),override);
    }else if(com=="il"){
        int i,j;string ss,acr;       
        if(!(line>>ss>>i>>j)) return;
        if(line) line>>acr; else acr='d';
        for(char &c:ss) c=toupper(c);
        Move m(ss,i,j,acr[0]!='d');
        bool leg=islegal(m);
        cout<<m<<" legal: "<<leg<<endl;
    }else if(com=="rm"){
        int i,j;
        if(!(line>>i>>j)) return;
        board[i][j]=points[i][j]=0;
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
                case 'p': print(points,"tile values:",3); break;
            }
        }
    }else if(com=="cl"){
        string ss;
        if(!(line>>ss)) return;
        ss[0]=toupper(ss[0]);
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
                    char*  t=find(rack,rack+racksize,toupper(ss[i]));
                    std::move(t+1,rack+racksize--,t);
                }
        }
        cout << "rack: ";
        for(int i=0;i<racksize;i++) cout<<rack[i];
        cout<<endl;
    }else if(com=="lm"){
        string st;
        int n=999999,s=0;
        while(line>>st) {
            if(st[0]!='-') continue;
            stringstream ss{st.substr(3)};
            switch(st[1]){
                case 'n':
                if(!(ss>>n)) n=999999;
                break;
                case 's':
                if(!(ss>>s)) s=0;
                break;
            }
        }
        findmoves();
        sort(moves.rbegin(),moves.rend());
        for(Move &m:moves){
            if(m.score<s || n--==0) break;
            cout <<m<<endl;
        }
    }else if(com=="db") line>>debug;
    else if(com=="file"){
        string name;if(!(line>>name)) return;
        fstream fs{name};
        if(!fs) cout <<name<<" not found"<<endl;
        while(fs){
            string cm;getline(fs,cm);
            docommand(cm);
        }
    }else if(com=="word"){
        string w;if(!(line>>w)) return;
        cout << isWord(w)<<endl;
    }else if(com=="clear"){
        for(int i=0;i<bsize+2;i++)
            for(int j=0;j<bsize+2;j++)
                if((i%(bsize+1))*(j%(bsize+1))==0) board[i][j]=1; else board[i][j]=0;
    }
}

int main(int argc, char* argv[]){
    for(int i=argc;i--;){
        char * s=argv[i];
        if(s[0]!='-') continue;
        stringstream ss(s+3);
        switch(s[1]){
            case 'b': ss>>boardfile; break;
            case 't': ss>>tilefile;break;
            case 'd': ss>>dictfile;break;
            case 'c': 
                string dir;ss>>dir;
                boardfile=dir+"/board.txt";
                tilefile=dir+"/tiles.txt";
                dictfile=dir+"/dict.txt";
                dictbin=dir+"/dict.bin";
                break;
        }
    }
    init();
    calcadj();
    while(cin){
        string s;getline(cin,s);
        docommand(s);
    }
}