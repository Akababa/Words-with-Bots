#include <iostream>
#include <cstring>
#include <iomanip>

// putting implementations in here because I don't want another file
class Move {
public:
    std::string word; //assume this is a valid word
    int length;
    int row, col;
    bool across;
    int score;
    int blank_pos; // which place the blank is in
    int heur_score = 0; //heuristic score
    int bsize; // board size
    std::string swap_letters;

    Move(int bsize) {
        this->bsize = bsize;
    }

    /*
    * Assume word passed in is valid
    */
    Move(const std::string &word, int r, int c, int bsize, bool across = true, int score = 0, int blank_pos = -1,
         int h = 0) {
        this->bsize = bsize;
        this->set_all(word, r, c, across, score, blank_pos, h);
    }

    void
    set_all(const std::string &word, int r, int c, bool across, int score, int blank_pos, int h = 0) {
        this->word = word;
        this->length = word.size();
        this->row = r;
        this->col = c;
        this->across = across;
        this->score = score;
        this->blank_pos = blank_pos;
        this->heur_score = h;
    }


    // Reads a line from stdin and parses it as a move into the passed Move object reference
    // returns: 0 for invalid input, 1 for successful move parse, 2 for swap, -1 for pass
    int input_move() {
        std::string sss;
        getline(std::cin, sss);
        std::stringstream line{sss};
        int i, j, p;
        std::string ss, acr;
        if (!(line >> ss)) return 0;
        if (ss == ".") return -1;
        if (ss == "!") {
            line >> this->swap_letters;
            return 2;
        }
        if (!(line >> i) || !(line >> j)) i = j = (bsize + 1) / 2;
        if (!(line >> acr)) acr = "a";
        if (!(line >> p)) p = -1;

        for (char &c:ss) c = toupper(c);
        set_all(ss, i, j, acr[0] != 'd', 0, p);
        return 1;
    }

    friend std::ostream &operator<<(std::ostream &out, Move &move);

    // comparison based on nominal score
    bool operator<(const Move &other) const {
        return (heur_score < other.heur_score) ||
               (heur_score == other.heur_score && std::string(word) < std::string(other.word));
    }
};

std::ostream &operator<<(std::ostream &out, Move &m) {
    using namespace std;
    out << m.word << " @" <<
        setw(2) << setfill(' ') << m.row << "," << setw(2) << setfill(' ') << m.col << (m.across ? " acr" : " dwn");
    if (m.score) out << " +" << m.score;
    if (m.heur_score) out << " (hscore:" << m.heur_score << ")";
    if (m.blank_pos != -1) out << " (blank:" << m.blank_pos << ")";
    return out;
}