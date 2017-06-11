#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <boost/dynamic_bitset.hpp>
using namespace std;
using namespace boost;
int main(){
	int depth=4;
	fstream fs("dict.txt");
	string s;
	boost::dynamic_bitset<> chains (27*27*27*27);
	while(fs>>s){
		for(char &c:s) c-=('A'-1); //1<=c<27

		chains[s[0]]=1;
		chains[s[0]*27+s[1]]=1;
		chains[(s[0]*27+s[1])*27+s[2]]=1;
		for (int i = 3; i < s.size(); ++i){
			chains[((s[i-3]*27+s[i-2])*27 +s[i-1])*27 +s[i]]=1;
		}
	}
	fs.close();

    std::vector<dynamic_bitset<>::block_type> v(chains.num_blocks());
    to_block_range(chains, v.begin());
    ofstream out("dict.bin", ios::out | ios::binary);
    out.write((char*)&depth,sizeof(int));
    int x=v.size();
    out.write((char*)&x,sizeof(int));
    out.write((char*)&v[0], x * sizeof(dynamic_bitset<>::block_type));
    out.close();


	ifstream in("dict.bin");
	int d,y;
	in.read((char*)&d,sizeof(int));
	in.read((char*)&y,sizeof(int));
    std::vector<dynamic_bitset<>::block_type> u(y);
	in.read((char*)&u[0],y*sizeof(dynamic_bitset<>::block_type));
	boost::dynamic_bitset<> xx(u.begin(),u.end());
	cout <<xx;

}
