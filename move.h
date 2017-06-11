#include <iostream>
#include <cstring>
#include <iomanip>
class Move {
public:
    char* word; //assume this is a valid word
    int length;
    int row,col;
    bool across;
    int score,rackused;
    int blankpos;

    /*
    * Assume word passed in is valid
    */
    private: Move(const char* word,int length,int r,int c,bool across=true, int score=0,int rackused=0,int blankpos=-1){
        this->word=new char[length+1];
        std::memcpy(this->word,word,length);
        this->word[length]=0;
        this->length=length;
    	this->row=r;
    	this->col=c;
        this->across=across;
    	this->score=score;
    	this->rackused=rackused;
        this->blankpos=blankpos;
    }

    public: Move(const std::string & word,int r,int c,bool across=true, int score=0,int rackused=0,int blankpos=-1): 
        Move(word.c_str(),word.size(),r,c,across,score,rackused,blankpos) {};

    friend std::ostream &operator<<(std::ostream &out, Move &move);

    bool operator< (const Move& other) const {
        return (score < other.score) || (score==other.score && std::string(word)<std::string(other.word));
    }
};

std::ostream &operator<<(std::ostream &out, Move &m){
    using namespace std;
    out<<setw(11)<<setfill(' ')<<m.word<<" @"<<
        setw(2)<<setfill(' ')<<m.row<<","<<setw(2)<<setfill(' ')<<m.col<<(m.across?" acrs":" down");
    if(m.score) out<<" worth "<<m.score;
    return out;
}