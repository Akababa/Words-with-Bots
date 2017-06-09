#include <iostream>
#include <string>

class Move {
public:
    std::string word; //assume this is a valid word
    int row,col;
    bool across;
    int score,rackused;

    /*
    * Assume word passed in is valid
    */
    Move(std::string word,int r,int c,bool across=true, int score=0,int rackused=0){
        this->word=word;
    	this->row=r;
    	this->col=c;
        this->across=across;
    	this->score=score;
    	this->rackused=rackused;
    }
    friend std::ostream &operator<<(std::ostream &out, Move &move);
};

std::ostream &operator<<(std::ostream &out, Move &m){
    return out<<m.word<<":"<<m.row<<","<<m.col<<(m.across?" across":" down");
}