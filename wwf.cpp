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
#include <cmath>
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

unsigned seed=0;
vector<char> bag;
vector<Move> moves;
char * rack=new char[999], *myrack=new char[999];
int racksize=0,myracksize=0;
bool fullrack;
int val[27];
int num[26];
int H1[27];
int bsize=15;
unsigned int depth=5,mod; //for pruning trie. mod=27^(depth)
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

void clear(){
    start=true;
    for(int i=0;i<bsize+2;i++)
        for(int j=0;j<bsize+2;j++){
            if((i%(bsize+1))*(j%(bsize+1))==0) board[i][j]=1; else board[i][j]=0;
            points[i][j]=0;
        }
}

bool isWord(const string& s){
    return words.count(s);
}

// Find the score of one word (across or down) at i,j or 0 if illegal
// nonadj = true finds score of non-anchored words
// illegal = true finds score of illegal words
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

int rackH1(){
    int sum=0;
    for(int i=racksize;i--;) sum+=H1[rack[i]-'A'];
    return sum;
}

void putmove(string accword,int i,int j,bool acr,int blankpos){
    int ii=i,jj=j;
    int adjscore=0;
    int wordscore=0;
    int wmult=1;
    int nn=0;
    char temp=accword[blankpos];
    if(blankpos!=-1) {
        accword[blankpos]='A'+26;
    }
    do{
        if(board[i][j]) {
            wordscore+=points[i][j];
        i+=!acr;j+=acr;
            continue;
        }
        adjscore+= wordmult[i][j] * 
        (adj[acr][i][j]? adj[acr][i][j] + letrmult[i][j] * val[accword[nn]-'A']:0);
        wordscore+= letrmult[i][j] * val[accword[nn]-'A']; 
        wmult*= wordmult[i][j];
        i+=!acr;j+=acr;
    }while(++nn<accword.size());
    if(start) adjscore=0;
    if(blankpos!=-1) accword[blankpos]=temp;
    int score=adjscore+wmult*wordscore;
    if(racksize==0 && fullrack) score+=35;
    //cout <<adjscore<<wordscore<<wmult;
    moves.push_back(Move(accword,ii,jj,acr,score,blankpos));
}

// accword is the cumulative word fragment
// accadj accumulates the adjacent word scores
// accletr accumulates the letter scores (incl. bonus)
// wmult is the cumulative word multiplier
// DONE: using full rack bonus, DONE: first move
string accword;
void findmovesat(int i,int j,bool acr,bool hasadj=false,int blankpos=-1,int rhash=0){
    if(board[i][j]){ // on a non-empty square
        if(board[i][j]==1) return;
        accword.push_back(board[i][j]);
        findmovesat(i+!acr,j+acr,acr,true,blankpos,((rhash+board[i][j]-'A'+1)*27)%mod); //passed letter doesn't count
        accword.pop_back();
        return;
    }
    const bool checkleaves=hasadj || adj[acr][i][j] || adj[!acr][i][j];
    // on an empty square
    for(int idc=racksize;idc--;) {
        if(rack[idc]-'A'==26){ //this is a blank //WARNING - RACK SIZE IS 1 LESS HERE
            rack[idc]=rack[--racksize];
            for(char c='A';c<='Z';c++){
                if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort
                if(chains[(rhash+c-'A'+1)]==0) continue; //impossible word, abort
                if(checkleaves){ //takes care of trailing letters
                        int ii=i,jj=j;
                        int siz=accword.size();
                        accword.push_back(c);
                        while(board[ii+=!acr][jj+=acr]>1){
                            accword.push_back(board[ii][jj]);
                        }
                        if(isWord(accword)){
                            putmove(accword,i-siz*!acr,j-siz*acr,acr,siz);
                        }
                    //if(accword=="TSADES" && siz==5 && acr) cout<<accadj<<" "<<accletr<<"+"<<bonus<<" "<<wmult<<" "<<wordmult[i][j]<<
                        //" "<<c<<" "<<i<<","<<j<<"  "<<blankpos<<endl;
                    accword.resize(siz);
                }
                if(racksize>0){
                    accword.push_back(c);
                    findmovesat(i+!acr, j+acr, acr, 
                        checkleaves,
                        accword.size()-1,
                        ((rhash+c-'A'+1)*27)%mod);
                    accword.pop_back();
                }
            }
            rack[racksize++]=rack[idc];
            rack[idc]='A'+26;
            continue;
        }
        char c=rack[idc];
        if(!legal[acr][c-'A'][i][j]) continue; //adjacent word illegal, abort //good chunk of time spent here
        if(chains[(rhash+c-'A'+1)]==0) continue; //impossible word, abort
            rack[idc]=rack[--racksize];
        if(checkleaves){ 
                int ii=i,jj=j;
                int siz=accword.size();
                accword.push_back(c);
                while(board[ii+=!acr][jj+=acr]>1){
                    accword.push_back(board[ii][jj]);
                }
                //if(accword=="ROADSTER")
                  //  cout << accword;
                if(isWord(accword)){
                    putmove(accword,i-siz*!acr,j-siz*acr,acr,blankpos);
                }
                accword.resize(siz);
        }
        if(racksize>0){
            accword.push_back(c);
            findmovesat(i+!acr, j+acr, acr, 
                checkleaves,
                blankpos,
                ((rhash+c-'A'+1)*27)%mod);
            accword.pop_back(); //new hotspot?
        }
            rack[racksize++]=rack[idc];
            rack[idc]=c;
    }
}

void findmoves(){
    calcadj();
    calclegalall();
    if(start) adj[0][6][6] = adj[1][6][6] = 1;
    fullrack=(racksize==7);
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
    mod=1; // for some reason 6 doesn't work TODO: find out why
    for(int i=depth;i--;mod*=27);
    fstream fs(dictfile);
    string s;
    chains=boost::dynamic_bitset<> (mod*27);
    while(fs>>s){
        for(char &c:s) c-=('A'-1); //1<=c<27
        int rhash=0;
        for (int i = 0; i < s.size(); ++i){
            rhash=(rhash*27+s[i])%mod;
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

void init(bool rebuild=false){
    fstream fs(dictfile);
    if(!fs){
        cout<<dictfile <<" not found, reverting to default"<<endl;
        fs.open("wwf/dict.txt");
    }
    words.clear();
    string s;
    while(fs){
        fs>>s;
        words.insert(s);
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
    shuffle (bag.begin(), bag.end(), default_random_engine(seed++));
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
    if(!rebuild && in){
        in.read((char*)&depth,sizeof(int));
        mod=1;
        for(int i=depth;i--;mod*=27);//mod=27^depth
        int y;
        in.read((char*)&y,sizeof(int));
        std::vector<dynamic_bitset<>::block_type> u(y);
        in.read((char*)&u[0],y*sizeof(dynamic_bitset<>::block_type));
        chains=dynamic_bitset<>(u.begin(),u.end());
    }else{
        in.close();
        writeIndex();
    }

    fs.open("wwf/H1.txt");
    while(fs){
        char c;int hscore;
        fs>>c>>hscore;
        H1[c-'A']=hscore;
    }

    clear();
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
    //cout<<"board:"<<endl;
    for(int i=0;i<=bsize;i++){
        for(int j=0;j<=bsize;j++){
            switch(board[i][j]){
                case 0:
                         if(wordmult[i][j]==3) cout <<'!';
                    else if(wordmult[i][j]==2) cout <<'?';
                    else if(letrmult[i][j]==3) cout <<'*';
                    else if(letrmult[i][j]==2) cout <<'\'';
                    else if(i==(bsize+1)/2&&j==(bsize+1)/2) cout <<'+';
                    else cout <<'.';
                    break;
                case 1: cout<<((i+j)%10); break;
                default: cout <<board[i][j];
            }
        }
        cout<<endl;
    }
}

// overwrite=true overwrites already placed tiles
void placemove(const Move &m,bool overwrite=false){
    start=false;
    for(int i=0,j=0; i+j<m.length; i+= !m.across,j+= m.across){
            if(!overwrite && board[m.row+i][m.col+j]) continue;

            board[m.row+i][m.col+j]=m.word[i+j];

            if(i+j!=m.blankpos)
                points[m.row+i][m.col+j]=val[m.word[i+j]-'A'];
    }
}

// inefficient islegal just for cmd line purpose
// mutates score and word
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

    //now it is legal, can mutate
    m.row=i1+!m.across; m.col = j1+m.across;
    m.word=acc;
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

void otherfindmoves(){
    std::swap(rack,myrack);
    std::swap(racksize,myracksize);
    findmoves();
    std::swap(racksize,myracksize);
    std::swap(rack,myrack);
}

// TODO: heuristics for better moves!
void sortmoves(bool useheur=true){
    for(Move &m: moves){
        if(!useheur){
            m.heurscore=m.score;
            continue;
        }
        //int hscore=m.score*100;

        //if(m.blankpos==-1)
            //hscore += 750; // bonus for not using the blank tile

        // I want to discourage use of rare letters - should have 2x multiplier or better
        for(int i=m.length;i--;){
            switch(m.word[i]){
                //case 'X': case 'Z': case 'J': case 'Q': hscore+=200; break;//I wanna keep these tiles
                //case 'S': case 'E': hscore-=200; break;
                //case 'H': hscore+=100; break;
                //case 'U': hscore+=100; break;
                //case 'S': hscore -=300;
                //case 'L': case 'U': case 'V': hscore -= 100;break;
            }
        }
        m.heurscore=m.heurscore/2+100*m.score;
    }

    sort(moves.rbegin(),moves.rend());
}
// control group (greedy)
void othersortmoves(){
    for(Move &m: moves){
        m.heurscore=m.score*100;

        //if(m.blankpos==-1)
            //m.heurscore += 750; // bonus for not using the blank tile

        // I want to discourage use of rare letters - should have x1.75 multiplier or better
        //for(int i=m.length;i--;){
            //m.heurscore -= 200 * val[m.word[i]-'A'];
        //}
    }

    sort(moves.rbegin(),moves.rend());
}
// returns: 0 for invalid input, 1 for succesful move parse, -1 for pass
int inputmove(Move &m){
    string sss; getline(cin,sss);
    stringstream line{sss};
    int i,j,p;string ss,acr;       
    if(!(line>>ss)) return 0;
    if(ss==".") return -1;
    if(!(line>>i) || !(line>>j)) i=j=(bsize+1)/2;
    if(!(line>>acr)) acr="a";
    if(!(line>>p)) p=-1;

    for(char &c:ss) c=toupper(c);
    m= Move (ss,i,j,acr[0]!='d',0,p);
    return islegal(m);
}

void docommand(string ssss, bool suppress=false);

void gameloop(){
    int stalemate=0;
    bool gameover=false;
    printboard();
    int myscore=0,botscore=0;
    while(true){
        // player turn
        for(int i=7-myracksize;i-- && !bag.empty();) {
            myrack[myracksize++]=bag.back();
            bag.pop_back();
        }
        if(myracksize==0) break;
        cout << "Your rack: ";
        for(int i=0;i<myracksize;i++) cout<<myrack[i];
        cout<<" Your score: "<<myscore<<" Bot score: "<<botscore<<endl;

        Move m; bool passed=false;
        while(true){
            cout<<"Input move (. to pass):";
            int status=inputmove(m);
            if(status==1) break;
            else if(status==-1) {
                cout<<"You passed"<<endl;
                passed=true;
                if(++stalemate>=3){
                    gameover=true;
                }
                break;
            }
        }
        if(gameover) break;
        if(!passed){
            stalemate=0;
            myscore+=m.score;
            int i=m.row,j=m.col;
            for(char c: m.word) {
                if(board[i][j]==0){
                    char * t=find(myrack,myrack+myracksize,toupper(c));
                   //cout <<t-myrack<<endl;
                    if(t==myrack+myracksize)
                        t=find(myrack,myrack+myracksize,'A'+26); // find the blank
                    
                    if(t!=myrack+myracksize)
                        std::move(t+1,myrack+myracksize--,t);
                }
                i+= !m.across; j+= m.across;
            }
            placemove(m);
            cout<<endl;
            printboard();
            cout <<"You played "<< m<<endl;
            cout<<endl;
        }
        // bot turn!

        for(int i=7-racksize;i-- && !bag.empty();) {
            rack[racksize++]=bag.back();
            bag.pop_back();
        }
        if(racksize==0) break;
        findmoves();
        sortmoves();
        bool botpassed=false;
        if(moves.empty()) {
            stalemate++;
            cout<<"Bot passes"<<endl;
            botpassed=true;
            if(++stalemate>=3){
                gameover=true;
            }
            continue;
        }
        if(gameover) break;
        if(!botpassed){
            stalemate=0;
            Move &bm=moves.front();
            botscore+=bm.score;
            int i=bm.row, j=bm.col;

            for(char c: bm.word) {
                if(board[i][j]==0){
                    char * t=find(rack,rack+racksize,toupper(c));
                    //cout <<t-myrack<<endl;
                    if(t==rack+racksize)
                        t=find(rack,rack+racksize,'A'+26); // find the blank
                    
                    if(t!=rack+racksize)
                        std::move(t+1,rack+racksize--,t);
                }
                i+= !bm.across; j+= bm.across;
            }
            placemove(bm);
            cout<<endl;
            printboard();
            cout <<"Bot played "<<bm<<endl;
            cout<<endl;
        }
    }
    if(stalemate<3){
        if(racksize==0){ // lose points for leftover letters
            for(int i=myracksize;i--;){
                myscore-=val[myrack[i]-'A'];
                botscore+=val[myrack[i]-'A'];
            }
        }else{
            for(int i=racksize;i--;){
                botscore-=val[rack[i]-'A'];
                myscore+=val[myrack[i]-'A'];
            }
        }
    }
    cout<<"Game over! Result: ";
    if(botscore>myscore) cout << "Bot wins";
    else if(myscore>botscore) cout << "You win";
    else cout <<"Tie";
    cout <<"\nYour score: "<<myscore<<" Bot score: "<<botscore<<endl;
}

int playprogram(bool gfrst, int verbosity=0){
    //printboard();
    int stalemate=0;
    bool gameover=false;
    int otherscore=0,botscore=0;
    while(true){
        if(!gfrst){
            // player turn
            for(int i=7-myracksize;i-- && !bag.empty();) {
                myrack[myracksize++]=bag.back();
                bag.pop_back();
            }
            if(myracksize==0) break;

            otherfindmoves();
            othersortmoves();
            bool opassed=false;
            if(moves.empty()) {
                stalemate++;
                if(verbosity>1) cout<<"."<<endl;
                opassed=true;
                if(++stalemate>=3){
                    gameover=true;
                }
            }
            if(gameover) break;
            if(!opassed){
                stalemate=0;
                Move &om =moves.front();
                otherscore+= om.score;
                int i=om.row;int j= om.col;
                for(char c: om.word) {
                    if(board[i][j]==0){
                        char * t=find(myrack,myrack+myracksize,toupper(c));
                       //cout <<t-myrack<<endl;
                        if(t==myrack+myracksize)
                            t=find(myrack,myrack+myracksize,'A'+26); // find the blank
                        
                        if(t!=myrack+myracksize)
                            std::move(t+1,myrack+myracksize--,t);
                    }
                    i+= !om.across; j+= om.across;
                }
                placemove(om);
                if(verbosity>1)cout<<om<<endl;

            }
        } else gfrst=false;

        // bot turn!

        for(int i=7-racksize;i-- && !bag.empty();) {
            rack[racksize++]=bag.back();
            bag.pop_back();
        }
        if(racksize==0) break;

        findmoves();
        sortmoves();
        bool botpassed=false;
        if(moves.empty()) {
            stalemate++;
            if(verbosity>1)cout<<"."<<endl;
            botpassed=true;
            if(++stalemate>=3){
                gameover=true;
            }
            continue;
        }
        if(gameover) break;
        if(!botpassed){
            stalemate=0;
            Move &bm=moves.front();
            botscore+=bm.score;
            int i=bm.row, j=bm.col;

            for(char c: bm.word) {
                if(board[i][j]==0){
                    char * t=find(rack,rack+racksize,toupper(c));
                    //cout <<t-myrack<<endl;
                    if(t==rack+racksize)
                        t=find(rack,rack+racksize,'A'+26); // find the blank
                    
                    if(t!=rack+racksize)
                        std::move(t+1,rack+racksize--,t);
                }
                i+= !bm.across; j+= bm.across;
            }
            placemove(bm);
            if(verbosity>1)cout<<bm<<endl;
        }
    }


    if(stalemate<3){
        if(racksize==0){ // lose points for leftover letters
            for(int i=myracksize;i--;){
                otherscore-=val[myrack[i]-'A'];
                botscore+=val[myrack[i]-'A'];
            }
        }else{
            for(int i=racksize;i--;){
                botscore-=val[rack[i]-'A'];
                otherscore+=val[myrack[i]-'A'];
            }
        }
    }
    if(verbosity>0)cout << botscore <<" VS "<<otherscore<<endl;
    if(botscore>otherscore) return 1;
    else if(botscore<otherscore) return -1;
    else return 0;
}

void docommand(string ssss, bool suppress){
    stringstream line(ssss);
    string com;line>>com;
    if(com=="game"){
        gameloop();
    }else if(com=="pb"){
        findmoves();
        if(moves.empty()) {
            if(!suppress) cout<<"No moves"<<endl;
            return;
        }
        sortmoves();
        Move & mm=moves.front();
        placemove(mm);
        if(!suppress){
            printboard();
            cout <<mm<<endl;
        }
        //docommand("ra -"+mm.word,suppress);
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
        placemove(Move(ss,i,j,acr[0]!='d',0,p),override);
    }else if(com=="il"){
        int i,j,p;string ss,acr;       
        if(!(line>>ss)) return;
        if(!(line>>i) || !(line>>j)) i=j=(bsize+1)/2;
        if(!(line>>acr)) acr="a";
        if(!(line>>p)) p=-1;

        for(char &c:ss) c=toupper(c);
        Move m(ss,i,j,acr[0]!='d',0,p);
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
                case 'b':
                    cout<<"board:"<<endl;
                    printboard();
                    break;
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
                    if(t!=rack+racksize)
                        std::move(t+1,rack+racksize--,t);
                }
        }
        if(suppress) return;
        cout << "rack: ";
        for(int i=0;i<racksize;i++) cout<<rack[i];
        cout<<endl;
    }else if(com=="lm"){
        string st;
        int n=999999,s=0;
        bool useheuristics=false;
        while(line>>st) {
            if(st[0]!='-') continue;
            stringstream ss;
            if(st.length()>=3) ss<<st.substr(3);
            switch(st[1]){
                case 'n':
                if(!(ss>>n)) n=999999;
                break;
                case 's':
                if(!(ss>>s)) s=0;
                break;
                case 'h': useheuristics=true;break;
            }
        }
        findmoves();
        sortmoves(useheuristics);
        for(Move &m:moves){
            if(m.score<s || n--==0) break;
            cout <<setw(11)<<setfill(' ')<<m<<endl;
        }
    }else if(com=="db") line>>debug;
    else if(com=="file"){
        string name;if(!(line>>name)) return;
        fstream fs{name};
        if(!fs) cout <<name<<" not found"<<endl;
        while(fs){
            string cm;getline(fs,cm);
            docommand(cm,suppress);
        }
    }else if(com=="word"){
        string w;if(!(line>>w)) return;
        cout << isWord(w)<<endl;
    }else if(com=="clear"){
        clear();
    }else if (com=="ip"){
        string s;line>>s;
        int rhash=0;
        for(char c:s){
            rhash=((rhash+toupper(c)-'A'+1)*27)%mod;
        }
        cout<<rhash<<":"<<chains[rhash]<<endl;
    }
}

int main(int argc, char* argv[]){
    bool rebuild=false;
    bool playingself=false;
    bool gofirst =true;
    int numgames,verbosity;
    for(int i=0; i<argc; i++){
        char * s=argv[i];
        if(s[0]!='-') continue;
        stringstream ss(s+3);
        stringstream sss;string ab;
        switch(s[1]){
            case 'b': ss>>boardfile; break;
            case 't': ss>>tilefile;break;
            case 'd': ss>>dictfile;break;
            case 'r': 
                if(!(ss>>depth)) depth = 5;
                rebuild=true;
                break;
            case 'p':
                playingself=true;
                ss>>numgames;
                verbosity=argv[3][0]-'0';
                //ab=string(argv[3]);
                //sss=stringstream{ab};
                //sss>>seed;
                if(argc==5) gofirst=false;
                break;
            case 'c': 
                string dir;ss>>dir;
                boardfile=dir+"/board.txt";
                tilefile=dir+"/tiles.txt";
                dictfile=dir+"/dict.txt";
                dictbin=dir+"/dict.bin";
                break;
        }
    }
    init(rebuild);
    if(playingself){
        int wins=0;
        const vector<char> baggy=bag;
        for(int i=0;i<numgames;i++){
            int res=playprogram(gofirst,verbosity);
            clear();
            bag=baggy;
            shuffle (bag.begin(), bag.end(), default_random_engine(seed++));
            racksize=myracksize=0;
            gofirst=!gofirst;
            wins+=(res==1);
            if(i%10==0){
                ifstream record("record.txt");
                int win1=0,tot1=0;record>>win1>>tot1;
                record.close();
                ofstream recor("record.txt",ios::trunc);
                recor<<win1+wins<<" "<<tot1+10;
                recor.close();
                wins=0;
            }
        }
        return 0;
    }
    calcadj();
    while(cin){
        string s;getline(cin,s);
        docommand(s,false);
    }
}