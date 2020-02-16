#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>
#include <bitset>
#include <sstream>
#include <algorithm>
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <cmath>
#include <cstdlib>
#include "move.h"

using namespace std;

bool debug = false;
bool start = true;

unordered_set<string> words; // hashtable dictionary
char **board; // TODO refactor this shit into a Board class
int ***adj; //scores of words in perpendicular directions
bool ****legal;
int **word_mult; // word multiplier bonus tiles
int **letter_mult; // letter multiplier bonus tiles
int **points; //point values of placed tiles
const int MOD = 27 * 27 * 27 * 27 * 27;
bitset<MOD> chains;

unsigned seed = chrono::system_clock::now().time_since_epoch().count();
vector<char> bag;
vector<Move> moves;
char *rack = new char[999], *my_rack = new char[999];
int rack_size = 0, my_rack_size = 0;
bool full_rack; // is my rack full at the start of my turn?
int val[27]; // letter values TODO refactor into a Bag class
int H1[27], H2[27][2]; // Rack leave heuristics
int bsize; // board size
string board_file = "wwf/board.txt"; // board layout
string tile_file = "wwf/tiles.txt"; // tiles in bag
string dict_file = "wwf/dict.txt", dict_bin = "wwf/dict.bin"; // dictionary and preprocessed dictionary

int score_word(int i, int j, bool across, bool non_adj, bool illegal);

//calculates the linear (no multipliers) scores of the word fragments already on the board
void calc_adj() {
    for (int i = 1; i <= bsize; i++)
        for (int j = 1; j <= bsize; j++)
            adj[0][i][j] = adj[1][i][j] = 0;
    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            if (board[i][j] == 0) {
                for (int ii = -1; ii <= 1; ii++) {
                    for (int jj = -1; jj <= 1; jj++) {
                        if ((ii + jj) % 2 && board[i + ii][j + jj] > 1) {
                            //board[i][j]='A'+26; // dummy character to discount ij
                            adj[ii * ii][i][j] = score_word(i, j, jj != 0, true, true);
                            //board[i][j]=0;
                        }
                    }
                }
            }
        }
    }
}

// zeros out the points
void clear() {
    start = true;
    for (int i = 0; i < bsize + 2; i++)
        for (int j = 0; j < bsize + 2; j++) {
            if ((i % (bsize + 1)) * (j % (bsize + 1)) == 0) board[i][j] = 1; else board[i][j] = 0;
            points[i][j] = 0;
        }
}

bool is_word(const string &s) {
    return words.count(s);
}

// Find the score of one linear word (across or down) at i,j or 0 if illegal
// non_adj = true finds score of non-anchored words
// illegal = true finds score of illegal words
string ret_word; //last checked word for convenience
int score_word(int i, int j, bool across, bool non_adj = false, bool illegal = false) {
    int score = 1; //in case word is full of blanks
    if (across) {
        if (non_adj || adj[0][i][j]) { //check horiz
            int j1 = j;
            while (board[i][--j1] > 1);
            int j2 = j;
            while (board[i][++j2] > 1);

            ret_word = string(j2 - j1 - 1, 0);
            for (int jj = j1 + 1; jj < j2; jj++) {
                ret_word[jj - j1 - 1] = board[i][jj];
                score += points[i][jj];
            }
            if (debug) cout << i << "," << j1 << "-" << j2 << ": " << ret_word << " " << is_word(ret_word) << endl;
            if (illegal) return score;
            if (!is_word(ret_word)) return 0;
            else return score;
        } else return 1;
    } else {
        if (non_adj || adj[1][i][j]) { //check vert
            int i1 = i;
            while (board[--i1][j] > 1);
            int i2 = i;
            while (board[++i2][j] > 1);
            ret_word = string(i2 - i1 - 1, 0);
            for (int ii = i1 + 1; ii < i2; ii++) {
                ret_word[ii - i1 - 1] = board[ii][j];
                score += points[ii][j];
            }
            if (debug) cout << i1 << "-" << i2 << "," << j << ": " << ret_word << " " << is_word(ret_word) << endl;
            if (illegal) return score;
            if (!is_word(ret_word)) return 0;
            else return score;
        } else return 1;
    }
    //remember to return true!!!
}

// Calculate all legal positions for one character
void calc_legal(char c) {
    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            if (board[i][j] == 0) {
                board[i][j] = c;
                //points[i][j]=val[c-'A']; //no need to set points if we only care about legality
                legal[0][c - 'A'][i][j] = score_word(i, j, true);
                legal[1][c - 'A'][i][j] = score_word(i, j, false);
                board[i][j] = 0;
                //points[i][j]=0;
            } else {
                legal[0][c - 'A'][i][j] = legal[1][c - 'A'][i][j] = false;
            }
        }
    }
}

// Calculate all legal positions for all characters
void calc_legal_all() {
    for (char c = 'A'; c <= 'Z'; c++) calc_legal(c);
}

// Simple rack leave (leftover) heuristic
int rackH1() {
    int sum = 0;
    for (int i = rack_size; i--;) sum += H1[rack[i] - 'A'];
    return sum;
}

// This heuristic deducts for duplicate letters
int rackH2() {
    int sum = 0;
    int freq[27];
    for (int i = 27; i--;) freq[i] = 0;
    for (int i = rack_size; i--;) {
        freq[rack[i] - 'A']++;
    }
    for (char c = 'A'; c <= 'A' + 26; c++) {
        if (freq[c - 'A'])
            sum += freq[c - 'A'] * (H2[c - 'A'][0] + (freq[c - 'A'] - 1) * H2[c - 'A'][1] / 2);
    }
    if (std::count(rack, rack + rack_size, 'U') && std::count(rack, rack + rack_size, 'Q')) sum += 1500;

    // vowels: A,E,I,O,U
    int num_vowels = freq[0] + freq['E' - 'A'] + freq['I' - 'A'] + freq['O' - 'A'] + freq['U' - 'A'];
    // consonants - vowels balance
    int bal = rack_size - num_vowels * 2 - 1; // ideally 3 vowels and 4 consonants
    sum -= bal * bal * 50; // max -2450 for 6 vowel leave
    return sum;
}

// puts a move on the moves list
void put_move(string acc_word, int i, int j, bool acr, int blank_pos) {
    int ii = i, jj = j;
    int adj_score = 0;
    int word_score = 0;
    int wmult = 1;
    int nn = 0;
    char temp = acc_word[blank_pos];
    if (blank_pos != -1) {
        acc_word[blank_pos] = 'A' + 26;
    }
    do {
        if (board[i][j]) {
            word_score += points[i][j];
            i += !acr;
            j += acr;
            continue;
        }
        adj_score += word_mult[i][j] *
                     (adj[acr][i][j] ? adj[acr][i][j] - 1 + letter_mult[i][j] * val[acc_word[nn] - 'A'] : 0);
        word_score += letter_mult[i][j] * val[acc_word[nn] - 'A'];
        wmult *= word_mult[i][j];
        i += !acr;
        j += acr;
    } while (++nn < acc_word.size());
    if (blank_pos != -1) acc_word[blank_pos] = temp;
    int score = adj_score + wmult * word_score;
    if (rack_size == 0 && full_rack) score += 35; //bingo
    moves.emplace_back(Move(acc_word, ii, jj, bsize, acr, score, blank_pos, rackH2()));
}

// acc_word is the cumulative word fragment
// accadj accumulates the adjacent word scores
// accletr accumulates the letter scores (incl. bonus)
// wmult is the cumulative word multiplier
// DONE: using full rack bonus, DONE: first move
string acc_word;

void find_moves_at(int i, int j, bool acr, bool has_adj = false, int blank_pos = -1, int r_hash = 0) {
    if (board[i][j]) { // on a non-empty square
        if (board[i][j] == 1) return;
        acc_word.push_back(board[i][j]);
        find_moves_at(i + !acr, j + acr, acr, true, blank_pos,
                      ((r_hash + board[i][j] - 'A' + 1) * 27) % MOD); //passed letter doesn't count
        acc_word.pop_back();
        return;
    }
    const bool check_leaves = has_adj || adj[acr][i][j] || adj[!acr][i][j];
    // on an empty square
    for (int idc = rack_size; idc--;) {
        if (rack[idc] - 'A' == 26) { //this is a blank //WARNING - RACK SIZE IS 1 LESS HERE
            rack[idc] = rack[--rack_size];
            for (char c = 'A'; c <= 'Z'; c++) {
                if (!legal[acr][c - 'A'][i][j]) continue; //adjacent word illegal, abort
                if (chains[(r_hash + c - 'A' + 1)] == 0) continue; //impossible word, abort
                if (check_leaves) { //takes care of trailing letters
                    int ii = i, jj = j;
                    int siz = acc_word.size();
                    acc_word.push_back(c);
                    while (board[ii += !acr][jj += acr] > 1) {
                        acc_word.push_back(board[ii][jj]);
                    }
                    if (is_word(acc_word)) {
                        put_move(acc_word, i - siz * !acr, j - siz * acr, acr, siz);
                    }
                    //if(acc_word=="TSADES" && siz==5 && acr) cout<<accadj<<" "<<accletr<<"+"<<bonus<<" "<<wmult<<" "<<word_mult[i][j]<<
                    //" "<<c<<" "<<i<<","<<j<<"  "<<blank_pos<<endl;
                    acc_word.resize(siz);
                }
                if (rack_size > 0) {
                    acc_word.push_back(c);
                    find_moves_at(i + !acr, j + acr, acr,
                                  check_leaves,
                                  acc_word.size() - 1,
                                  ((r_hash + c - 'A' + 1) * 27) % MOD);
                    acc_word.pop_back();
                }
            }
            rack[rack_size++] = rack[idc];
            rack[idc] = 'A' + 26;
            continue;
        }
        char c = rack[idc];
        if (!legal[acr][c - 'A'][i][j]) continue; //adjacent word illegal, abort //good chunk of time spent here
        if (chains[(r_hash + c - 'A' + 1)] == 0) continue; //impossible word, abort
        rack[idc] = rack[--rack_size];
        if (check_leaves) {
            int ii = i, jj = j;
            int siz = acc_word.size();
            acc_word.push_back(c);
            while (board[ii += !acr][jj += acr] > 1) {
                acc_word.push_back(board[ii][jj]);
            }
            //if(acc_word=="ROADSTER")
            //  cout << acc_word;
            if (is_word(acc_word)) {
                put_move(acc_word, i - siz * !acr, j - siz * acr, acr, blank_pos);
            }
            acc_word.resize(siz);
        }
        if (rack_size > 0) {
            acc_word.push_back(c);
            find_moves_at(i + !acr, j + acr, acr,
                          check_leaves,
                          blank_pos,
                          ((r_hash + c - 'A' + 1) * 27) % MOD);
            acc_word.pop_back(); //new hotspot?
        }
        rack[rack_size++] = rack[idc];
        rack[idc] = c;
    }
}

void find_moves() {
    calc_adj();
    calc_legal_all();
    if (start) adj[0][6][6] = adj[1][6][6] = 1;
    full_rack = (rack_size == 7);
    moves.clear();
    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            if (board[i][j] == 0) { //only look on free squares to avoid redundancy
                int j1 = j;
                while (board[i][--j1] > 1); //go to leftmost taken square on board
                find_moves_at(i, j1 + 1, true);
                int i1 = i;
                while (board[--i1][j] > 1);
                find_moves_at(i1 + 1, j, false);
            }
        }
    }
}

// Writes the preprocessed dict trie into a file TODO refactor this into a separate file
void write_index() {
    // for some reason 6 doesn't work TODO: find out why
    fstream fs(dict_file);
    string s;
    while (fs >> s) {
        for (char &c:s) c -= ('A' - 1); //1<=c<27
        int rhash = 0;
        for (char c : s) {
            rhash = (rhash * 27 + c) % MOD;
            chains[rhash] = true;
        }
    }
    fs.close();

    ofstream out(dict_bin, ios::out | ios::binary);
    out.write((char *) &chains, sizeof(bitset<MOD>));
    out.close();
}

// Initializes the bot
void init(bool rebuild = false) {
    fstream fs(dict_file);
    if (!fs) {
        cout << dict_file << " not found, reverting to default" << endl;
        fs.open("wwf/dict.txt");
    }
    words.clear();
    string s;
    while (fs) {
        fs >> s;
        words.insert(s);
    }
    fs.close();

    fs.open(tile_file);
    if (!fs) {
        cout << tile_file << " not found, reverting to default" << endl;
        fs.open("wwf/tiles.txt");
    }
    bag.clear();
    while (fs) {
        char ch;
        int number, value;
        fs >> ch >> number >> value;
//        num[ch - 'A'] = number;
        val[ch - 'A'] = value;
        for (int i = number; i--;) bag.push_back(ch);
    }
    unsigned my_seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle(bag.begin(), bag.end(), default_random_engine(my_seed));
    //for(int i:val) cout <<i <<" "<<endl;
    fs.close();

    fs.open(board_file);
    if (!fs) {
        cout << board << " not found, reverting to default" << endl;
        fs.open("wwf/board.txt");
    }
    fs >> bsize;
    int bt = bsize + 2;
    adj = new int **[2]; //scores of words in perpendicular directions
    adj[0] = new int *[bt];
    adj[1] = new int *[bt];
    legal = new bool ***[2];
    legal[0] = new bool **[27];
    legal[1] = new bool **[27];
    for (int j = 0; j < 27; j++) {
        legal[0][j] = new bool *[bt];
        legal[1][j] = new bool *[bt];
    }

    board = new char *[bt];
    word_mult = new int *[bt];
    letter_mult = new int *[bt];
    points = new int *[bt];
    for (int i = 0; i < bt; ++i) {
        board[i] = new char[bt];
        letter_mult[i] = new int[bt];
        word_mult[i] = new int[bt];
        points[i] = new int[bt];
        adj[0][i] = new int[bt];
        adj[1][i] = new int[bt];
        for (int j = 0; j < 27; j++) {
            legal[0][j][i] = new bool[bt];
            legal[1][j][i] = new bool[bt];
        }
    }

    for (int i = 1; i <= bsize; i++)
        for (int j = 1; j <= bsize; j++)
            letter_mult[i][j] = 1;
    for (int i = 1; i <= bsize; i++)
        for (int j = 1; j <= bsize; j++)
            word_mult[i][j] = 1;

    while (fs) {
        int mult;
        fs >> mult;
        string lw;
        fs >> lw;
        int n;
        fs >> n;
        if (lw[0] == 'L') {
            for (int i = n; i--;) {
                int a, b;
                fs >> a >> b;
                letter_mult[a][b] = letter_mult[a][bsize + 1 - b] = letter_mult[bsize + 1 - a][b] = letter_mult[bsize +
                                                                                                                1 - a][
                        bsize + 1 - b] = mult;
            }
        } else {
            for (int i = n; i--;) {
                int a, b;
                fs >> a >> b;
                word_mult[a][b] = word_mult[a][bsize + 1 - b] = word_mult[bsize + 1 - a][b] = word_mult[bsize + 1 - a][
                        bsize + 1 - b] = mult;
            }
        }
    }
    fs.close();

    ifstream in(dict_bin);
    if (!rebuild && in) {
        in.read((char *) &chains, sizeof(bitset<MOD>));
    } else {
        in.close();
        write_index();
    }

    fs.open("wwf/H1.txt");
    while (fs) {
        char c;
        int hscore;
        fs >> c >> hscore;
        H1[c - 'A'] = hscore;
    }
    fs.close();

    fs.open("wwf/H2.txt");
    while (fs) {
        char c;
        int s1, s2;
        fs >> c >> s1 >> s2;
        H2[c - 'A'][0] = s1;
        H2[c - 'A'][1] = s2;
    }

    clear();
}

void print(bool **arr, const string &s) {
    cout << s << endl;
    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            cout << arr[i][j];
        }
        cout << endl;
    }
}

void print(int **arr, const string &s, int width = 1) {
    cout << s << endl;
    for (int i = 1; i <= bsize; i++) {
        for (int j = 1; j <= bsize; j++) {
            cout << setfill(' ') << setw(width) << arr[i][j];
        }
        cout << endl;
    }
}

// Prints the scrabble board TODO into the Board class
void print_board() {
    //cout<<"board:"<<endl;
    for (int i = 0; i <= bsize; i++) {
        for (int j = 0; j <= bsize; j++) {
            cout << " ";
            switch (board[i][j]) {
                case 0:
                    if (word_mult[i][j] == 3) cout << '!';
                    else if (word_mult[i][j] == 2) cout << '?';
                    else if (letter_mult[i][j] == 3) cout << '*';
                    else if (letter_mult[i][j] == 2) cout << '\'';
                    else if (i == (bsize + 1) / 2 && j == (bsize + 1) / 2) cout << '+';
                    else cout << '.';
                    break;
                case 1:
                    if (i == 0 && j == 0) {
                        cout << " ";
                    } else {
                        cout << ((i + j) % 10);
                    }
                    break;
                default:
                    cout << board[i][j];
            }
        }
        cout << endl;
    }
}

// overwrite=true overwrites already placed tiles
// TODO move into a Board class
void place_move(const Move &m, bool overwrite = false) {
    start = false;
    for (int i = 0, j = 0; i + j < m.length; i += !m.across, j += m.across) {
        if (!overwrite && board[m.row + i][m.col + j]) continue;

        board[m.row + i][m.col + j] = m.word[i + j];

        if (i + j != m.blank_pos)
            points[m.row + i][m.col + j] = val[m.word[i + j] - 'A'];
    }
}

// inefficient is_legal just for cmd line purpose
// mutates score and word
// modifies m if it's legal, otherwise returns false
// Does not mutate the board (!) :D
// doesn't work with blanks, nbd for now
// TODO try to move this into the Board class once it exists
bool is_legal(Move &m) {
    if (m.word.empty()) return false;
    calc_adj();
    calc_legal_all();
    int i = m.row, j = m.col;
    for (int nn = 0; nn < m.length; nn++, i += !m.across, j += m.across) {
        if (board[i][j] == 1) return false;
        if (board[i][j] == m.word[nn]) continue;
        if (board[i][j] > 1) return false;
        if (!legal[m.across][m.word[nn] - 'A'][i][j]) return false;
    }
    string acc = m.word;
    int i1 = m.row, j1 = m.col;
    while (board[i1 -= !m.across][j1 -= m.across] > 1)
        acc = string(1, board[i1][j1]) + acc;

    while (board[i][j] > 1) {
        acc.push_back(board[i][j]);
        i += !m.across;
        j += m.across;
    }
    if (!is_word(acc)) return false;

    //now it is legal, can mutate
    m.row = i1 + !m.across;
    m.col = j1 + m.across;
    m.word = acc;
    int adj_score = 0;
    int word_score = 0;
    int w_mult = 1;
    int nn = 0;
    //cout <<i1<<j1<<i<<j<<endl;
    do {
        i1 += !m.across;
        j1 += m.across;
        if (board[i1][j1]) {
            word_score += val[acc[nn] - 'A'];
            continue;
        }
        adj_score += word_mult[i1][j1] *
                     (adj[m.across][i1][j1] ? adj[m.across][i1][j1] - 1 + letter_mult[i1][j1] * val[acc[nn] - 'A'] : 0);
        word_score += letter_mult[i1][j1] * val[acc[nn] - 'A'];
        w_mult *= word_mult[i1][j1];
    } while (++nn < acc.size());
    //cout <<adj_score<<word_score<<w_mult;


    m.score = adj_score + w_mult * word_score;
    return true;
}

void other_find_moves() {
    std::swap(rack, my_rack);
    std::swap(rack_size, my_rack_size);
    find_moves();
    std::swap(rack_size, my_rack_size);
    std::swap(rack, my_rack);
}

// TODO: heuristics for better moves!
void sort_moves(bool use_heur = true) {
    for (Move &m: moves) {
        if (!use_heur) {
            m.heur_score = m.score;
            continue;
        }

        // weight rack leave by number of tiles left in bag
        int si = bag.size();
        m.heur_score = m.heur_score * si / 100 + 100 * m.score;
    }

    sort(moves.rbegin(), moves.rend());
}

// control group for testing heuristics
void other_sort_moves() {
    for (Move &m: moves) {
        m.heur_score = m.score * 100;
    }

    sort(moves.rbegin(), moves.rend());
}


void do_command(string &cmd_string, bool suppress = false);

void game_loop() {
    int stalemate = 0;
    bool game_over = false;
    print_board();
    int my_score = 0, bot_score = 0;
    while (true) {
        // player turn
        for (int i = 7 - my_rack_size; i-- && !bag.empty();) {
            my_rack[my_rack_size++] = bag.back();
            bag.pop_back();
        }
        if (my_rack_size == 0) break;
        cout << "Your rack: ";
        for (int i = 0; i < my_rack_size; i++) cout << my_rack[i];
        cout << " Your score: " << my_score << " Bot score: " << bot_score << endl;

        Move m(bsize);
        bool passed = false;
        int status = 0;
        while (status == 0) {
            cout << "Input move (. to pass, ! to swap): ";
            status = m.input_move();
            if (!is_legal(m)) {
                status = 0;
            }
            if (status == 2) {
                for (char &c:m.swap_letters) {
                    char *pos = find(my_rack, my_rack + my_rack_size, toupper(c));
                    if (pos != rack + rack_size) {
                        std::swap(my_rack[pos - my_rack], bag[rand() % bag.size()]);
                    }
                }
                passed = true;
                break;
            } else if (status == -1) {
                cout << "You passed" << endl;
                passed = true;
                if (++stalemate >= 3) {
                    game_over = true;
                }
            } else if (status == 0) {
                cout << "Invalid move, try again." << endl;
            }
        }
        if (game_over) break;
        if (!passed) {
            stalemate = 0;
            my_score += m.score;
            int i = m.row, j = m.col;
            for (char c: m.word) {
                if (board[i][j] == 0) {
                    char *t = find(my_rack, my_rack + my_rack_size, toupper(c));
                    //cout <<t-my_rack<<endl;
                    if (t == my_rack + my_rack_size)
                        t = find(my_rack, my_rack + my_rack_size, 'A' + 26); // find the blank

                    if (t != my_rack + my_rack_size)
                        std::move(t + 1, my_rack + my_rack_size--, t);
                }
                i += !m.across;
                j += m.across;
            }
            place_move(m);
            cout << endl;
            print_board();
            cout << "You played " << m << endl;
            cout << endl;
        }
        // bot turn!

        for (int i = 7 - rack_size; i-- && !bag.empty();) {
            rack[rack_size++] = bag.back();
            bag.pop_back();
        }
        if (rack_size == 0) break;
        find_moves();
        sort_moves();
        bool bot_passed = false;
        if (moves.empty()) {
            stalemate++;
            cout << "Bot passes" << endl;
            bot_passed = true;
            if (++stalemate >= 3) {
                game_over = true;
            }
            continue;
        }
        if (game_over) break;
        if (!bot_passed) {
            stalemate = 0;
            Move &bm = moves.front();
            bot_score += bm.score;
            int i = bm.row, j = bm.col;

            for (char c: bm.word) {
                if (board[i][j] == 0) {
                    char *t = find(rack, rack + rack_size, toupper(c));
                    //cout <<t-my_rack<<endl;
                    if (t == rack + rack_size)
                        t = find(rack, rack + rack_size, 'A' + 26); // find the blank

                    if (t != rack + rack_size)
                        std::move(t + 1, rack + rack_size--, t);
                }
                i += !bm.across;
                j += bm.across;
            }
            place_move(bm);
            cout << endl;
            print_board();
            cout << "Bot played " << bm << endl;
            cout << endl;
        }
    }
    if (stalemate < 3) {
        if (rack_size == 0) { // lose points for leftover letters
            for (int i = my_rack_size; i--;) {
                my_score -= val[my_rack[i] - 'A'];
                bot_score += val[my_rack[i] - 'A'];
            }
        } else {
            for (int i = rack_size; i--;) {
                bot_score -= val[rack[i] - 'A'];
                my_score += val[my_rack[i] - 'A'];
            }
        }
    }
    cout << "Game over! Result: ";
    if (bot_score > my_score) cout << "Bot wins";
    else if (my_score > bot_score) cout << "You win";
    else cout << "Tie";
    cout << "\nYour score: " << my_score << " Bot score: " << bot_score << endl;
}

// plays the bot against itself many times, for testing
int play_program(bool go_first, int verbosity = 0) {
    //print_board();
    int stalemate = 0;
    bool game_over = false;
    int other_score = 0, bot_score = 0;
    while (true) {
        if (!go_first) {
            // player turn
            for (int i = 7 - my_rack_size; i-- && !bag.empty();) {
                my_rack[my_rack_size++] = bag.back();
                bag.pop_back();
            }
            if (my_rack_size == 0) break;

            other_find_moves();
            other_sort_moves();
            bool opassed = false;
            if (moves.empty()) {
                stalemate++;
                if (verbosity > 1) cout << "." << endl;
                opassed = true;
                if (++stalemate >= 3) {
                    game_over = true;
                }
            }
            if (game_over) break;
            if (!opassed) {
                stalemate = 0;
                Move &om = moves.front();
                other_score += om.score;
                int i = om.row;
                int j = om.col;
                for (char c: om.word) {
                    if (board[i][j] == 0) {
                        char *t = find(my_rack, my_rack + my_rack_size, toupper(c));
                        //cout <<t-my_rack<<endl;
                        if (t == my_rack + my_rack_size)
                            t = find(my_rack, my_rack + my_rack_size, 'A' + 26); // find the blank

                        if (t != my_rack + my_rack_size)
                            std::move(t + 1, my_rack + my_rack_size--, t);
                    }
                    i += !om.across;
                    j += om.across;
                }
                place_move(om);
                if (verbosity > 1)cout << om << endl;

            }
        } else go_first = false;

        // bot turn!

        for (int i = 7 - rack_size; i-- && !bag.empty();) {
            rack[rack_size++] = bag.back();
            bag.pop_back();
        }
        if (rack_size == 0) break;

        find_moves();
        sort_moves();
        bool bot_passed = false;
        if (moves.empty()) {
            stalemate++;
            if (verbosity > 1)cout << "." << endl;
            bot_passed = true;
            if (++stalemate >= 3) {
                game_over = true;
            }
            continue;
        }
        if (game_over) break;
        if (!bot_passed) {
            stalemate = 0;
            Move &bm = moves.front();
            bot_score += bm.score;
            int i = bm.row, j = bm.col;

            for (char c: bm.word) {
                if (board[i][j] == 0) {
                    char *t = find(rack, rack + rack_size, toupper(c));
                    //cout <<t-my_rack<<endl;
                    if (t == rack + rack_size)
                        t = find(rack, rack + rack_size, 'A' + 26); // find the blank

                    if (t != rack + rack_size)
                        std::move(t + 1, rack + rack_size--, t);
                }
                i += !bm.across;
                j += bm.across;
            }
            place_move(bm);
            if (verbosity > 1)cout << bm << endl;
        }
    }

    if (stalemate < 3) {
        if (rack_size == 0) { // lose points for leftover letters
            for (int i = my_rack_size; i--;) {
                other_score -= val[my_rack[i] - 'A'];
                bot_score += val[my_rack[i] - 'A'];
            }
        } else {
            for (int i = rack_size; i--;) {
                bot_score -= val[rack[i] - 'A'];
                other_score += val[my_rack[i] - 'A'];
            }
        }
    }
    if (verbosity > 0)cout << bot_score << " VS " << other_score << endl;
    if (bot_score > other_score) return 1;
    else if (bot_score < other_score) return -1;
    else return 0;
}

// Executes the passed cmd_string
void do_command(string &cmd_string, bool suppress) {
    stringstream line(cmd_string);
    string command;
    line >> command;
    if (command == "game") {
        game_loop();
    } else if (command == "pb") { // play best move (according to bot)
        find_moves();
        if (moves.empty()) {
            if (!suppress) cout << "No moves" << endl;
            return;
        }
        sort_moves();
        Move &mm = moves.front();
        place_move(mm);
        if (!suppress) {
            print_board();
            cout << mm << endl;
        }
        //do_command("ra -"+mm.word,suppress);
    } else if (command == "pm") { // Play move
        bool override = false;
        int i, j, p;
        string ss, acr;
        if (!(line >> ss)) return;
        if (ss == "-f") {
            override = true;
            if (!(line >> ss)) return;
        }
        if (!(line >> i) || !(line >> j)) i = j = (bsize + 1) / 2;
        if (!(line >> acr)) acr = "a";
        if (!(line >> p)) p = -1;

        for (char &c:ss) c = toupper(c);
        place_move(Move(ss, i, j, bsize, acr[0] != 'd', 0, p), override);
    } else if (command == "il") { // List legal moves
        int i, j, p;
        string ss, acr;
        if (!(line >> ss)) return;
        if (!(line >> i) || !(line >> j)) i = j = (bsize + 1) / 2;
        if (!(line >> acr)) acr = "a";
        if (!(line >> p)) p = -1;

        for (char &c:ss) c = toupper(c);
        Move m(ss, i, j, bsize, acr[0] != 'd', 0, p);
        bool leg = is_legal(m);
        cout << m << " legal: " << leg << endl;
    } else if (command == "rm") { // remove a tile from the board
        int i, j;
        if (!(line >> i >> j)) return;
        board[i][j] = 0;
        points[i][j] = 0;
    } else if (command == "pr") {
        string ss;
        line >> ss;
        for (char cc:ss) {
            switch (cc) {
                case 'b': // print the board
                    cout << "board:" << endl;
                    print_board();
                    break;
                case 'a': // prints the scores of adjacent words
                    print(adj[0], "adj[0]:", 3);
                    print(adj[1], "adj[1]:", 3);
                    break;
                case 'r': // print my rack
                    cout << "rack: ";
                    for (int i = 0; i < rack_size; i++) cout << rack[i];
                    cout << endl;
                    break;
                case 'l': // print letter bonus squares
                    print(letter_mult, "letter multipliers:");
                    break;
                case 'w': // print word bonus squares
                    print(word_mult, "word multipliers:");
                    break;
                case 'p': // print tile values
                    print(points, "tile values:", 3);
                    break;
                default:
                    cout << "Invalid print command: " << cc << endl;
                    break;
            }
        }
    } else if (command == "cl") {
        string ss;
        if (!(line >> ss)) return;
        ss[0] = toupper(ss[0]);
        calc_legal(ss[0]);
        print(legal[0][ss[0] - 'A'], "legal[0][" + ss + "]");
        print(legal[1][ss[0] - 'A'], "legal[1][" + ss + "]");
    } else if (command == "swap") {
        string ss;
        if (!(line >> ss)) return;
        for (char &c:ss) {
            char *pos = find(rack, rack + rack_size, toupper(c));
            if (pos != rack + rack_size) {
                std::swap(rack[pos - rack], bag[rand() % bag.size()]);
            }
        }
    } else if (command == "ra") {
        while (line) {
            string ss;
            line >> ss;
            if (ss[0] == '+') {
                if (isdigit(ss[1]))
                    for (int i = ss[1] - '0'; i-- && !bag.empty();) {
                        rack[rack_size++] = bag.back();
                        bag.pop_back();
                    }
                else
                    for (char &c:ss)
                        rack[rack_size++] = toupper(c);
            }
            if (ss[0] == '-') {
                for (int i = 1; i < ss.size(); i++) {
                    char *pos = find(rack, rack + rack_size, toupper(ss[i]));
                    if (pos != rack + rack_size)
                        std::move(pos + 1, rack + rack_size--, pos);
                }
            }
        }
        if (suppress) return;
        cout << "rack: ";
        for (int i = 0; i < rack_size; i++) cout << rack[i];
        cout << endl;
    } else if (command == "lm") {
        string st;
        int n = 999999, s = 0;
        bool use_heuristics = false;
        while (line >> st) {
            if (st[0] != '-') continue;
            stringstream ss;
            if (st.length() >= 3) ss << st.substr(3);
            switch (st[1]) {
                case 'n':
                    if (!(ss >> n)) n = 999999;
                    break;
                case 's':
                    if (!(ss >> s)) s = 0;
                    break;
                case 'h':
                    use_heuristics = true;
                    break;
            }
        }
        find_moves();
        sort_moves(use_heuristics);
        for (Move &m:moves) {
            if (m.score < s || n-- == 0) break;
            cout << setw(11) << setfill(' ') << m << endl;
        }
    } else if (command == "db") {
        line >> debug;
    } else if (command == "file") {
        string name;
        if (!(line >> name)) return;
        fstream fs{name};
        if (!fs) cout << name << " not found" << endl;
        while (fs) {
            string cm;
            getline(fs, cm);
            do_command(cm, suppress);
        }
    } else if (command == "word") {
        string w;
        if (!(line >> w)) return;
        for (char &c:w) c = toupper(int(c));
        cout << is_word(w) << endl;
    } else if (command == "clear") {
        clear();
    } else if (command == "ip") {
        string s;
        line >> s;
        int r_hash = 0;
        for (char c:s) {
            r_hash = ((r_hash + toupper(c) - 'A' + 1) * 27) % MOD;
        }
        cout << r_hash / 27 << ":" << chains[r_hash / 27] << endl;
    } else {
        cout << "Invalid command, try again." << endl;
    }
}

int main(int argc, char *argv[]) {
    bool rebuild = false;
    bool playing_self = false;
    bool go_first = true;
    int num_games = -1, verbosity;
    for (int i = 0; i < argc; i++) {
        char *s = argv[i];
        if (s[0] != '-') continue;
        stringstream ss(s + 3);
        stringstream sss;
        string ab;
        switch (s[1]) {
            case 'b':
                ss >> board_file;
                break;
            case 't':
                ss >> tile_file;
                break;
            case 'd':
                ss >> dict_file;
                break;
            case 'r':
                rebuild = true;
                break;
            case 'p':
                playing_self = true;
                ss >> num_games;
                verbosity = argv[3][0] - '0';
                //ab=string(argv[3]);
                //sss=stringstream{ab};
                //sss>>seed;
                if (argc == 5) go_first = false;
                break;
            case 'c':
                string dir;
                ss >> dir;
                board_file = dir + "/board.txt";
                tile_file = dir + "/tiles.txt";
                dict_file = dir + "/dict.txt";
                dict_bin = dir + "/dict.bin";
                break;
        }
    }
    init(rebuild);
    if (playing_self) {
        int wins = 0;
        const vector<char> baggy = bag;
        for (int i = 0; i < num_games; i++) {
            int res = play_program(go_first, verbosity);
            clear();
            bag = baggy;
            shuffle(bag.begin(), bag.end(), default_random_engine(seed++));
            rack_size = my_rack_size = 0;
            go_first = !go_first;
            wins += (res == 1);
            if (i % 10 == 0) {
                ifstream record("record.txt");
                int win1 = 0, tot1 = 0;
                record >> win1 >> tot1;
                record.close();
                ofstream recor("record.txt", ios::trunc);
                recor << win1 + wins << " " << tot1 + 10;
                recor.close();
                wins = 0;
            }
        }
        return 0;
    }
    calc_adj();
    while (cin) {
        string s;
        getline(cin, s);
        do_command(s, false);
    }
}
