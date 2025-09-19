#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <cstdio>
#include <unistd.h>	   // for fork(), execvp()
#include <sys/types.h> // for pid_t
#include <sys/wait.h>  // for waitpid()
#include <vector>
#include <fcntl.h>
#include <unordered_map>
#include <cstring>
#include <algorithm> //for sorting
// #include <bits/stdc++.h>
#include <termios.h>
#include <cstdlib>

using namespace std;
using namespace std::filesystem; // find out why this and why not for other headers

// need trie to store names of all executables
typedef struct Trie_node
{
	unordered_map<char, struct Trie_node *> children;
	bool isEnd = false;
} TrieNode;

class Trie
{

public:
	TrieNode *root = new TrieNode();

	void insert(string word)
	{
		TrieNode *node = root;
		for (char c : word)
		{
			// node not present
			if (!node->children[c])
				node->children[c] = new TrieNode();
			node = node->children[c];
		}
		node->isEnd = true;
	}
};

// storing all executable names in a trie
void make_trie(Trie &trie)
{
	string path_env = getenv("PATH");
	stringstream ss(path_env);

	string path;

	// inside loop there is a variation of try and catch
	// for better code....refer it later
	while (getline(ss, path, ':'))
	{
		for (auto it : directory_iterator(path))
		{
			string entry_path = it.path().string();
			string filename = it.path().filename().string();
			//&& access(entry_path.c_str(),X_OK
			if (is_regular_file(entry_path))
			{
				// file is executable
				trie.insert(filename);
			}
		}
	}
}
string find_path(string cmd)
{
	string path_env = getenv("PATH");
	stringstream ss(path_env);

	string path;

	while (getline(ss, path, ':'))
	{
		string cmd_path = path + "/" + cmd;

		if (is_regular_file(cmd_path) && access(cmd_path.c_str(), X_OK) == 0)
		{
			return cmd_path; // return on first match
		}
	}
	return "";
}

// make a cleaner version if possible
vector<string> split(string s)
{
	vector<string> res;

	// below iis to strip off white space
	int start = 0;
	while (s[start] == ' ' && start < s.size())
		start++;

	string a = "";
	int sq = 0, dq = 0, pres = 0;
	for (int i = start; i < s.size(); i++)
	{
		if (s[i] == ' ' && sq == 0 && dq == 0 && pres == 0)
		{
			if (!a.empty())
			{
				res.push_back(a);
				a = "";
			}
		}
		else if (s[i] == '\'')
		{
			if (sq == 0 && dq == 0 && pres == 0)
				sq = 1;
			else if (sq == 1 && pres == 0)
				sq = 0;
			else
			{
				if (pres == 1 && dq == 1)
					a += '\\';
				a += s[i];
				pres = 0;
			}
		}

		else if (s[i] == '\"')
		{
			if (sq == 0 && dq == 0 && pres == 0)
				dq = 1;
			else if (dq == 1 && pres == 0)
				dq = 0;
			else
			{
				a += s[i];
				pres = 0;
			}
		}

		else if (s[i] == '\\')
		{
			if (sq == 0 && pres == 0)
				pres = 1;
			else
			{
				a += s[i];
				pres = 0;
			}
		}

		else
		{
			if (dq == 1 && pres == 1)
				a += '\\';
			a += s[i];
			pres = 0;
		}
	}

	if (!a.empty())
	{
		res.push_back(a);
	}
	return res;
}

// below fn is to change terminal settings
//  comment more explanation here later
void setRawMode(bool enable)
{
	static struct termios oldt, newt;
	static bool inRaw = false;

	if (enable)
	{
		if (!inRaw)
		{
			tcgetattr(STDIN_FILENO, &oldt); // Save old terminal settings
			inRaw = true;
		}
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);		 // Disable canonical mode and echo
		tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Apply new settings
	}
	else
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
	}
}

// removed echo as its also present as executable
// so to prevent conflict
vector<string> built_ins = {"exit", "type", "cd", "pwd"};

void find_built_ins(string text, vector<string> &res)
{
	int len = text.size();
	for (int i = 0; i < built_ins.size(); i++)
	{
		if (strncmp(text.c_str(), built_ins[i].c_str(), len) == 0)
		{
			res.push_back(built_ins[i]);
		}
	}
}

void get_all_execs(TrieNode *node, string prefix, vector<string> &res)
{
	// ie we have already typed the whole word and no more other
	// word that are possible from what we have typed
	if (node->isEnd)
		res.push_back(prefix);
	for (auto it : node->children)
	{
		char c = it.first;
		TrieNode *child = it.second;

		get_all_execs(child, prefix + c, res);
	}
}

void find_execs(string text, vector<string> &res, Trie &trie)
{
	int len = text.size();
	TrieNode *node = trie.root;
	for (char c : text)
	{
		if (!node->children[c])
			return;
		node = node->children[c];
	}

	get_all_execs(node, text, res);
}

void partial_comp(string &text, Trie &trie)
{
	TrieNode *node = trie.root;
	for (char c : text)
	{
		node = node->children[c];
	}
	while (node->children.size() == 1 && node->isEnd == false)
	{
		char letter = node->children.begin()->first;
		text += letter;
		node = node->children[letter];
	}
}

void autocomplete(string &input, int &found, Trie &trie, int &tab)
{
	string copy = input;
	string last_word;
	for (int i = copy.size() - 1; i >= 0; i--)
	{
		if (copy[i] == ' ' || copy.empty())
			break;
		last_word = copy[i] + last_word;
		copy.pop_back();
	}

	vector<string> matches;
	find_built_ins(last_word, matches);
	find_execs(last_word, matches, trie);
	// sorting if needed just find words in order in prev algos
	// so as to prevent sorting....this is temporary
	sort(matches.begin(), matches.end());

	if (matches.size() == 0)
	{
		// if no matches
		// \a is bell sounnd??? or something
		cout << "\r\a$ " << input << flush;
		tab = 0;
	}
	else if (matches.size() == 1)
	{
		copy += matches[0] + " ";
		cout << "\r$ " << copy << flush;
		input = copy;
		found = 1;
	}
	else
	{
		if (tab == 1)
		{
			// check if we can partial complete;
			partial_comp(last_word, trie);
			copy += last_word;
			cout << "\r$ " << copy << flush;
			input = copy;
			tab++;
			return;
		}

		tab = 1;
		cout << "\n"
			 << flush;
		for (int i = 0; i < matches.size(); i++)
		{
			if (i % 5 == 4)
				cout << matches[i] << "\n"
					 << flush;
			else
				cout << matches[i] << "  " << flush;
		}
		// cout<<"\ntoo many matches\n"<<flush;
		cout << "\n\r\a$ " << input << flush;
	}
}

int execute(string input)
{
	// cout<<input<<endl;
	vector<string> parsed = split(input);
	// cout<<parsed[0]<<endl<<parsed[1]<<endl;

	vector<string> output;
	// to split parsed again to accomodate redirection
	int redirection = 0;
	int error_redirection = 0;
	int append = 0;
	int i = 0;
	while (i < parsed.size())
	{

		if (parsed[i] == ">" || parsed[i] == "1>" || parsed[i] == "2>" ||
			parsed[i] == ">>" || parsed[i] == "2>>" || parsed[i] == "1>>")
		{

			redirection = 1;
			if (parsed[i][0] == '2')
				error_redirection = 1;
			if (parsed[i] == "2>>" || parsed[i] == "1>>" || parsed[i] == ">>")
				append = 1;
			parsed.erase(parsed.begin() + i);
		}
		else if (redirection == 1)
		{
			output.push_back(parsed[i]);
			parsed.erase(parsed.begin() + i);
		}
		else
			i++;
	}

	string cmd = parsed[0];

	int terminal; // to used to store like fd so we can redirect o/p
	int fd = -1;  // file desc
	// to terminal

	// stops on "exit";
	if (cmd == "exit")
		return 0;

	// echo cmd to print text after it
	else if (cmd == "echo")
	{

		if (redirection)
		{
			// O_WRONLY we are opening file in write only mode if it exists
			//  if file doesnt exists O_CREAT creates a file and permission 0644
			//  if already exits then O_TRUNC clears old contents
			if (!append)
				fd = open(output[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			else
				fd = open(output[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0664);
			if (fd < 0)
			{
				perror("open failed");
				return 1;
			}
			// below we store to redirect back to terminal
			terminal = dup(STDOUT_FILENO);
			// redirect stdout to the file
			if (error_redirection)
				dup2(fd, STDERR_FILENO);
			else
				dup2(fd, STDOUT_FILENO);
		}

		for (int i = 1; i < parsed.size(); i++)
		{
			cout << parsed[i];
			if (i < parsed.size() - 1)
				cout << " ";
		}
		cout << endl;

		if (redirection && fd >= 0)
		{
			close(fd);
			// changing redirection back to terminal
			dup2(terminal, STDOUT_FILENO);
			dup2(terminal, STDERR_FILENO);
		}
	}

	else if (cmd == "type")
	{
		for (int i = 1; i < parsed.size(); i++)
		{
			string cmd2 = parsed[i];
			if (cmd2 == "echo" || cmd2 == "exit" || cmd2 == "type" || cmd2 == "pwd")
			{
				cout << cmd2 << " is a shell builtin" << endl;
			}
			else
			{
				// cout<<cmd2<<": not found"<<endl;

				string path = find_path(cmd2);
				if (path.empty())
				{
					cout << cmd2 << ": not found\n";
				}
				else
				{
					cout << cmd2 << " is " << path << endl;
				}
			}
		}
	}

	else if (cmd == "pwd")
	{
		cout << current_path().string() << endl; // gives pwd
	}
	// std::cout<<input<<": command not found" << std::endl;

	else if (cmd == "cd")
	{
		// below works for relative and absolute paths
		// there is another way but below is similar to what is used by actual shell
		string dest = parsed[1];
		if (dest == "~")
		{
			dest = getenv("HOME");
		}

		int fail = chdir(dest.c_str()); // chdir changes current working dir
		if (fail != 0)
		{
			cout << "cd: " << dest << ": No such file or directory" << endl;
		}
	}

	else
	{
		string path = find_path(cmd);

		if (path == "")
			cout << cmd << ": command not found" << endl;

		else
		{
			// we will be making a fork and running exec in that

			pid_t pid = fork();

			if (pid == -1)
			{
				cerr << "fork failed" << endl;
			}
			else if (pid == 0)
			{
				// child process

				string executable = parsed[0];

				// execvp(const char* command, char* argv[]);
				char ex[executable.size() + 1];
				for (int i = 0; i < executable.size(); i++)
					ex[i] = executable[i];
				ex[executable.size()] = '\0';
				// execvp needs \0 at end of string

				// to accomodate the format of execvp
				char *arg[parsed.size() + 1];
				for (int i = 0; i < parsed.size(); i++)
				{
					// below comments didnt as memory is not alloced
					//  const char *p=parsed[i+1].c_str();
					//  strcpy(arg[i],p);
					arg[i] = strdup(parsed[i].c_str());
				}
				arg[parsed.size()] = nullptr;
				// execvp needs array to end with null pointer;

				if (redirection)
				{
					// O_WRONLY we are opening file in write only mode if it exists
					//  if file doesnt exists O_CREAT creates a file and permission 0644
					//  if already exits then O_TRUNC clears old contents
					//  O_APPEND appends content
					if (!append)
						fd = open(output[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
					else
						fd = open(output[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0664);

					if (fd < 0)
					{
						perror("open failed");
						return 1;
					}
					// below we store to redirect back to terminal
					terminal = dup(STDOUT_FILENO);

					// redirect stdout/stderr to the file
					if (error_redirection)
						dup2(fd, STDERR_FILENO);
					else
						dup2(fd, STDOUT_FILENO);
				}

				execvp(ex, arg);
			}
			else
			{ // parent process
				wait(NULL);
				if (redirection && fd >= 0)
				{
					close(fd);
					// changing redirection back to terminal
					dup2(terminal, STDOUT_FILENO);
					dup2(terminal, STDERR_FILENO);
				}
			}
		}
	}
	// for now keep this return value
	return 1;
}

int main()
{

	// Flush after every std::cout / std:cerr

	// std::cout << std::unitbuf;
	cout << std::unitbuf;
	// std::cerr << std::unitbuf;
	cerr << std::unitbuf;

	Trie trie;
	make_trie(trie);

	while (1)
	{

		cout << "$ ";
		string input;
		// // std::getline(std::cin, input);
		// getline(cin,input);

		// below readline method to get i/p is to
		// accomodate tab autocompletion
		// char *ip=readline("$ ");

		// understand setRawMode again
		setRawMode(true);
		// this disables echo so you cant see what you type in the screen
		// we have to manually display this

		int found = 0;
		int tab = 0; // for tab count
		while (true)
		{
			// cout<<"entered\n";
			char ch = getchar();

			if (ch == '\n')
			{
				cout << "\n";
				tab = 0;
				found = 1;
				break;
			}
			else if (ch == '\t')
			{
				tab++;
				autocomplete(input, found, trie, tab);
				// break;
			}
			else if (ch == 127)
			{
				tab = 0;
				// this is for backspace
				if (!input.empty())
				{
					input.pop_back();
					//\r is to clear the line
					cout << "\r$ " << input;
					// below space is to replace the backspaced char
					// even though we reprint a stirng of len 0 to n-2
					// what we had removed in n-1 will still be displayed
					// so that is replaced by space
					cout << ' ';
					cout << "\r$ " << input << flush;
				}
			}
			else
			{
				tab = 0;
				input += ch;
				cout << ch << flush;
			}
		}

		setRawMode(false);

		if (input.empty() || !found)
			continue;

		vector<string> cmds;
		stringstream ss(input);
		string part;

		while (getline(ss, part, ';'))
		{
			cmds.push_back(part);
		}

		for (int i = 0; i < cmds.size(); i++)
		{
			// fn to exec a single instr
			//  this was done bcoz one line could be multiple commands
			//  so as to accomodate executing all of them
			//  eg ls;ls
			//  stringstream ss(cmds[i]);
			vector<string> pipe_split;
			// while(getline(ss,part,'|')){
			//         pipe_split.push_back(part);
			// }
			string s = "";

			for (int j = 0; j < cmds[i].size(); j++)
			{

				if (cmds[i][j] == '|')
				{
					if (!s.empty())
						pipe_split.push_back(s);
					s = "";
				}
				else
				{
					s += cmds[i][j];
					// cout<<s<<endl;
				}
			}
			if (!s.empty())
				pipe_split.push_back(s);
			if (pipe_split.size() <= 1)
			{
				int exit = execute(cmds[i]);
				if (!exit)
					return exit;
			}

			else
			{
				int n = pipe_split.size();

				vector<int> pipefds(2 * (n - 1));
				for (int i = 0; i < n - 1; i++)
				{
					if (pipe(pipefds.data() + i * 2) == -1)
					{
						perror("pipe");
						return 1;
					}
				}

				vector<pid_t> pids(n);

				for (int i = 0; i < n; i++)
				{
					pids[i] = fork();
					if (pids[i] == -1)
					{
						perror("fork");
						return 1;
					}

					if (pids[i] == 0)
					{

						if (i > 0)
						{
							dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
						}

						if (i < n - 1)
						{
							dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
						}

						for (int j = 0; j < 2 * (n - 1); j++)
							close(pipefds[j]);

						int ext = execute(pipe_split[i]);
						return 1;
					}
				}

				for (int j = 0; j < 2 * (n - 1); j++)
					close(pipefds[j]);

				for (int i = 0; i < n; i++)
					waitpid(pids[i], nullptr, 0);
			}
		}
	}
}