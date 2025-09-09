#include<bits/stdc++.h>
#include<cstdlib>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<sstream>

using namespace std;

struct Command{
	string name;
	vector<string> args;
};

// <--- Utilities ----> //
Command parseInput(const string input){
	Command cmd;
	istringstream inp(input);
	string token;
	if(inp >> token){
		cmd.name = token;
	}
	while(inp >> token){
		cmd.args.push_back(token);
	}
	return cmd;
}

bool isBuiltin(const string &name){
	return(name == "exit" || name == "echo" || name == "type" || name=="pwd");
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

void handlePwd(const Command &cmd) {
    char cwd[1024];  // buffer
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		cout << cwd << "\n";
    } 	else {
		perror("getcwd");
    }
}


void handleExit(const Command &cmd){
	int code = (cmd.args.empty() ? 0 : stoi(cmd.args[0]));
	exit(code);
}

void handleEcho(const Command &cmd){
	if(!cmd.args.empty()){
		for(auto &arg: cmd.args){
			cout<< arg<<" ";
		}
	}
	cout<<'\n';
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
	else if(cmd.name == "pwd") handlePwd(cmd);
	else {
		vector<char*> argv;
		argv.push_back(const_cast<char*>(cmd.name.c_str()));
		for(auto &arg : cmd.args){
			argv.push_back(const_cast<char*>(arg.c_str()));
		}
		argv.push_back(NULL);

		pid_t pid = fork();
		if(pid == 0){
			// <--- child process:
			execvp(argv[0], argv.data());
			cerr<< cmd.name<<": command not found\n";
			exit(1);

		}
		else if (pid > 0) {
	        // <--- Parent process: wait for child
        		int status;
        		waitpid(pid, &status, 0);
    		} else {
        		perror("fork failed");
    		}
	}


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
