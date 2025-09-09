#include<bits/stdc++.h>
#include<cstdlib>

using namespace std;

struct Command{
	string name;
	vector<string> args;
};

// <--- Utilities ----> //
Command parseInput(const string input){
	Command cmd;
	//int size = sizeof(input)/sizeof(string);
	int pos = input.find(" ");
	if(pos== string::npos) cmd.name = input;
	else{
		cmd.name = input.substr(0, pos);
		string rest = input.substr(pos+1);
		cmd.args.push_back(rest);
	}
	return cmd;
}

bool isBuiltin(const string &name){
	return(name == "exit" || name == "echo" || name == "type");
}

//<--- Search PATH --->
void searchPath(const string &commandName) {
    char* pathenv = getenv("PATH");
    if (!pathenv) return;

    string path = pathenv;
    vector<string> dirs;
    stringstream ss(path);
    string dir;

    while (getline(ss, dir, ':')) {
        string candidate = dir + "/" + commandName;
        if (access(candidate.c_str(), X_OK) == 0) {
            cout << commandName << " is " << candidate << "\n";
            return;
        }
    }

    cout << commandName << ": not found\n";
}

//<--- Builtin handlers ---> //

void handleExit(const Command &cmd){
	int code = (cmd.args.empty() ? 0 : stoi(cmd.args[0]));
	exit(code);
}

void handleEcho(const Command &cmd){
	if(!cmd.args.empty()){
		cout<< cmd.args[0]<<'\n';
	}
}

void handleType(const Command &cmd) {
	if (cmd.args.empty()) return;

	string arg = cmd.args[0];
	if (isBuiltin(arg)) {
		cout << arg << " is a shell builtin\n";
    	} else {
        	searchPath(arg);	
	}
}


//<--- Dispatcher ---> //
void execute(const Command &cmd){
	if (cmd.name == "exit") handleExit(cmd);
	else if (cmd.name == "echo") handleEcho(cmd);
	else if (cmd.name == "type") handleType(cmd);
	else cout << cmd.name << ": command not found\n";
}

// <--- Main --->
int main(){
	cout<<std:: unitbuf;
	cerr<<std::unitbuf;
	cout<<"$ ";
	string input;
	if(!getline(cin,input)) return 0;

	Command cmd = parseInput(input);
	execute(cmd);
	main();
	return 0;
}
