#include <iostream>
#include <cstring>

class Move {
public:
    char* word; //assume this is a valid word
    int length;
    int row,col;
    bool across;
    int score,rackused;

    /*
    * Assume word passed in is valid
    */
    Move(const char* word,int length,int r,int c,bool across=true, int score=0,int rackused=0){
        this->word=new char[length+1];
        std::memcpy(this->word,word,length);
        this->word[length]=0;
        this->length=length;
    	this->row=r;
    	this->col=c;
        this->across=across;
    	this->score=score;
    	this->rackused=rackused;
    }

    Move(std::string word,int length,int r,int c,bool across=true, int score=0,int rackused=0): 
        Move(word.c_str(),length,r,c,across,score,rackused) {};
    friend std::ostream &operator<<(std::ostream &out, Move &move);
    bool operator< (const Move& other) const {
        return (score < other.score);
    }
};

std::ostream &operator<<(std::ostream &out, Move &m){
    out<<m.word<<" @ "<<m.row<<","<<m.col<<(m.across?" across":" down");
    if(m.score) out<<" worth "<<m.score;
    return out;
}