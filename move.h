#include <iostream>
#include <cstring>
#include <iomanip>

// putting implementations in here because I don't want another file
class Move {
public:
    std::string word; //assume this is a valid word
    int length;
    int row,col;
    bool across;
    int score;
    int blankpos;
    int heurscore=0; //heuristic score
    Move() = default;
    /*
    * Assume word passed in is valid
    */
     Move(std::string word,int r,int c,bool across=true, int score=0,int blankpos=-1,int h=0){
        this->word=word;
        this->length=word.size();
    	this->row=r;
    	this->col=c;
        this->across=across;
    	this->score=score;
        this->blankpos=blankpos;
        this->heurscore=h;
    }

    friend std::ostream &operator<<(std::ostream &out, Move &move);

    // comparison based on nominal score
    bool operator< (const Move& other) const {
        return (heurscore < other.heurscore) || (heurscore==other.heurscore && std::string(word)<std::string(other.word));
    }
};

std::ostream &operator<<(std::ostream &out, Move &m){
    using namespace std;
    out<<m.word<<" @"<<
        setw(2)<<setfill(' ')<<m.row<<","<<setw(2)<<setfill(' ')<<m.col<<(m.across?" acr":" dwn");
    if(m.score) out<<" for "<<m.score;
    if(m.heurscore) out<<" (hscore:"<<m.heurscore<<")";
    if(m.blankpos!=-1) out<<" (blank:"<<m.blankpos<<")";
    return out;
}