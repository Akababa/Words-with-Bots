#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
using namespace std;

int main(){
	fstream fs; fs.open("wwfdict.txt");
	vector<string> all;
	while(fs){
		string s;fs>>s;
		all.push_back(s);
	}
	cout <<all.size()<<" words found"<<endl;
	unordered_map<string,vector<int>> stuff;
	for(char c='a';c<='z';c++){
		string ss(1,c);
		cout << "Building "<<ss<<endl;
		for(int i=0;i<all.size();i++){
			if(all[i].find(c)){
				stuff[ss].push_back(i);
			}
		}
	}
	for(char c='a';c<='z';c++){
		for(char d=c+1;d<='z';d++){
			string ss(1,c);
			vector<int> cstrings=stuff[ss];
			ss.push_back(d);
			cout << "Building "<<ss<<endl;
			for(int i=0;i<cstrings.size();i++){
				if(all[cstrings[i]].find(d)) stuff[ss].push_back(cstrings[i]);
			}
		}
	}
	for(char c='a';c<='z';c++){
		for(char d=c+1;d<='z';d++){
			for(char e=d+1;e<='z';e++){
				string ss(1,c);ss.push_back(d);
				vector<int> cdstrings=stuff[ss];
				ss.push_back(e);
				cout << "Building "<<ss<<endl;
				for(int i=0;i<cdstrings.size();i++){
					if(all[cdstrings[i]].find(e)) stuff[ss].push_back(cdstrings[i]);
				}
			}
		}
	}
	for(auto kv:stuff){
		ofstream f; f.open("data/"+kv.first, ios::out | ios::binary);
		f.write((char*)&kv.second[0],kv.second.size()*sizeof(int));
		f.close();
	}
}