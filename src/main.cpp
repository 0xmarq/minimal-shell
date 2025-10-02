#include <bits/stdc++.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <termios.h>
#include <dirent.h>

using namespace std;

struct Trie
{
	struct Node
	{
		unordered_map<char, Node *> children;
		bool is_end = false;
	};
	Node *root = new Node();

	void insert(const string &word)
	{
		Node *node = root;
		for (char c : word)
		{
			if (!node->children.count(c))
				node->children[c] = new Node();
			node = node->children[c];
		}
		node->is_end = true;
	}

	// Returning all completions for a given prefix....
	void dfs(Node *node, string &path, vector<string> &results)
	{
		if (node->is_end)
			results.push_back(path);

		// Collect keys and sort them
		vector<char> keys;
		for (auto &it : node->children)
			keys.push_back(it.first);
		sort(keys.begin(), keys.end());

		for (char c : keys)
		{
			path.push_back(c);
			dfs(node->children[c], path, results);
			path.pop_back();
		}
	}

	vector<string> complete(const string &prefix)
	{
		Node *node = root;
		for (char c : prefix)
		{
			if (!node->children.count(c))
				return {};
			node = node->children[c];
		}
		vector<string> results;
		string path = prefix;
		dfs(node, path, results);
		return results;
	}
};

struct Command
{
	string name;
	vector<string> args;
};

// Enable raw mode for terminal
void setRawMode(bool enable)
{
	static struct termios oldt, newt;
	if (enable)
	{
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO); // Dude wtf is this???
		// Im saving the old attributes and disabling canonical mode and echo
		// Then setting the new attributes
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	}
	else
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	}
}

//<--- Search PATH --->
void searchPath(const string &commandName)
{
	char *pathenv = getenv("PATH");
	if (!pathenv)
		return;

	string path = pathenv;
	vector<string> dirs;
	stringstream ss(path);
	string dir;

	while (getline(ss, dir, ':'))
	{
		string candidate = dir + "/" + commandName;
		if (access(candidate.c_str(), X_OK) == 0)
		{
			cout << commandName << " is " << candidate << "\n";
			return;
		}
	}

	cout << commandName << ": not found\n";
}

string longestCommonPrefix(const vector<string> &matches)
{
	if (matches.empty())
		return "";
	string prefix = matches[0];
	for (size_t i = 1; i < matches.size(); i++)
	{
		size_t j = 0;
		while (j < prefix.size() && j < matches[i].size() && prefix[j] == matches[i][j])
			++j;
		prefix = prefix.substr(0, j);
		if (prefix.empty())
			break;
	}
	return prefix;
}
class Parser
{
public:
	vector<string> tokenize(const string &input)
	{
		vector<string> tokens;
		string token;
		bool in_double_quotes = false;
		bool in_single_quotes = false;

		for (size_t i = 0; i < input.size(); ++i)
		{
			char c = input[i];

			if (c == '"' && !in_single_quotes)
			{
				in_double_quotes = !in_double_quotes;
			}
			else if (c == '\'' && !in_double_quotes)
			{
				in_single_quotes = !in_single_quotes;
			}
			else if (c == '\\')
			{
				if (i + 1 < input.size())
				{
					char next_c = input[++i];

					if (in_double_quotes || in_single_quotes)
					{
						// inside quotes → honor escapes
						switch (next_c)
						{
						case 'n':
							token += '\n';
							break;
						case 't':
							token += '\t';
							break;
						case 'r':
							token += '\r';
							break;
						case '\\':
							token += '\\';
							break;
						case '\'':
							token += '\'';
							break;
						case '"':
							token += '"';
							break;
						default:
							token += next_c;
							break;
						}
					}
					else
					{
						// outside quotes → keep literal
						token += next_c;
					}
				}
				else
				{
					token += '\\'; // trailing backslash
				}
			}
			else if (isspace(c) && !in_double_quotes && !in_single_quotes)
			{
				if (!token.empty())
				{
					tokens.push_back(token);
					token.clear();
				}
			}
			else
			{
				token += c;
			}
		}

		if (!token.empty())
		{
			tokens.push_back(token);
		}

		return tokens;
	}

	Command parseInput(const string input)
	{
		Command cmd;
		vector<string> tokens = tokenize(input);
		if (!tokens.empty())
		{
			cmd.name = tokens[0];
			for (size_t i = 1; i < tokens.size(); ++i)
				cmd.args.push_back(tokens[i]);
		}
		return cmd;
	}
	vector<Command> parsePipe(const string input)
	{
		vector<Command> commands;
		istringstream inp(input);
		string segment;
		while (getline(inp, segment, '|'))
		{
			commands.push_back(parseInput(segment));
		}
		return commands;
	}
};

class BuiltInHandler
{
public:
	bool isBuiltin(const string &name)
	{
		return (name == "exit" || name == "echo" || name == "type" || name == "pwd");
	}
	//<--- Builtin handlers ---> //

	void handleExit(const Command &cmd)
	{
		int code = (cmd.args.empty() ? 0 : stoi(cmd.args[0]));
		exit(code);
	}
	void handleEcho(const Command &cmd)
	{
		for (size_t i = 0; i < cmd.args.size(); i++)
		{
			if (i > 0)
				write(STDOUT_FILENO, " ", 1);
			write(STDOUT_FILENO, cmd.args[i].c_str(), cmd.args[i].size());
		}
		write(STDOUT_FILENO, "\n", 1);
	}

	void handleType(const Command &cmd)
	{
		if (cmd.args.empty())
			return;

		string arg = cmd.args[0];
		if (isBuiltin(arg))
		{
			cout << arg << " is a shell builtin\n";
		}
		else
		{
			searchPath(arg);
		}
	}
	void handlePwd(const Command &cmd)
	{
		char cwd[1024]; // buffer
		if (getcwd(cwd, sizeof(cwd)) != NULL)
		{
			cout << cwd << "\n";
		}
		else
		{
			perror("getcwd");
		}
	}

	void handleCd(const Command &cmd)
	{
		const char *path;
		int flag = 0;
		if (cmd.args.empty())
		{
			cout << " cd: missing argument" << '\n';
			flag = 1;
		}
		else if (cmd.args[0] == "~")
		{
			path = getenv("HOME");
		}
		else
		{
			path = cmd.args[0].c_str();
		}
		if (chdir(path) != 0 && flag == 0)
		{
			cerr << "cd: " << path << ": " << strerror(errno) << '\n';
		}
	}
};

class Executor
{
public:
	// Helper: convert vector<string> to char* array for execvp
	char **vec_to_cstr_array(const vector<string> &v) // Really clueless what this actually does....
	{
		char **arr = new char *[v.size() + 1];
		for (size_t i = 0; i < v.size(); ++i)
		{
			arr[i] = strdup(v[i].c_str());
		}
		arr[v.size()] = nullptr;
		return arr;
	}

	// New: pipeline for two commands only
	void executePipeline(const vector<Command> &commands)
	{
		int n = commands.size();
		if (n < 2)
			return;

		vector<int> pipes(2 * (n - 1));
		for (int i = 0; i < n - 1; ++i)
		{
			if (pipe(&pipes[2 * i]) < 0)
			{
				perror("pipe");
				exit(EXIT_FAILURE);
			}
		}

		vector<pid_t> pids(n);
		for (int i = 0; i < n; ++i)
		{
			pids[i] = fork();
			if (pids[i] == 0)
			{
				setvbuf(stdout, NULL, _IONBF, 0);
				setvbuf(stderr, NULL, _IONBF, 0);
				if (i > 0)
				{
					dup2(pipes[2 * (i - 1)], STDIN_FILENO);
				}
				if (i < n - 1)
				{
					dup2(pipes[2 * i + 1], STDOUT_FILENO);
				}
				for (int j = 0; j < 2 * (n - 1); ++j)
					close(pipes[j]);
				BuiltInHandler built;
				if (built.isBuiltin(commands[i].name))
				{
					if (commands[i].name == "echo")
						built.handleEcho(commands[i]);
					else if (commands[i].name == "pwd")
						built.handlePwd(commands[i]);
					else if (commands[i].name == "type")
						built.handleType(commands[i]);
					_exit(0);
				}

				// If not a builtin, execute external command
				// External command
				vector<string> args;
				args.push_back(commands[i].name);
				args.insert(args.end(), commands[i].args.begin(), commands[i].args.end());
				char **argv = vec_to_cstr_array(args);
				execvp(argv[0], argv);
				for (size_t k = 0; argv[k] != nullptr; ++k)
					free(argv[k]);
				delete[] argv;
				perror("execvp");
				exit(EXIT_FAILURE);
				perror("execvp");
				exit(EXIT_FAILURE);
			}
			else if (pids[i] < 0)
			{
				perror("fork");
				exit(EXIT_FAILURE);
			}
		}

		// Parent closes all pipe fds
		for (int j = 0; j < 2 * (n - 1); ++j)
			close(pipes[j]);
		// Wait for all children
		for (int i = 0; i < n; ++i)
			waitpid(pids[i], nullptr, 0);
	}

	//<--- Dispatcher ---> //
	void execute(const Command &cmd)
	{
		BuiltInHandler b;
		if (cmd.name == "exit")
		{
			b.handleExit(cmd);
			cout.flush();
			cerr.flush();
		}
		else if (cmd.name == "echo")
			b.handleEcho(cmd);
		else if (cmd.name == "type")
			b.handleType(cmd);
		else if (cmd.name == "pwd")
			b.handlePwd(cmd);
		else if (cmd.name == "cd")
			b.handleCd(cmd);
		else
		{
			vector<char *> argv;
			argv.push_back(const_cast<char *>(cmd.name.c_str()));
			for (auto &arg : cmd.args)
			{
				argv.push_back(const_cast<char *>(arg.c_str()));
			}
			argv.push_back(NULL);

			pid_t pid = fork();
			if (pid == 0)
			{
				// <--- child process:
				execvp(argv[0], argv.data());
				cerr << cmd.name << ": command not found\n";
				exit(1);
			}
			else if (pid > 0)
			{
				// <--- Parent process: wait for child
				int status;
				waitpid(pid, &status, 0);
			}
			else
			{
				perror("fork failed");
			}
		}
	}
};

class Shell
{
	Trie trie;

public:
	Shell()
	{
		// Insert builtins
		vector<string> builtins = {"exit", "echo", "type", "pwd", "cd"};
		for (const string &cmd : builtins)
		{
			trie.insert(cmd);
		}
		char *pathenv = getenv("PATH");
		if (pathenv)
		{
			string path = pathenv;
			stringstream ss(path);
			string dir;
			while (getline(ss, dir, ':'))
			{
				DIR *dp = opendir(dir.c_str());
				if (dp)
				{
					struct dirent *entry;
					while ((entry = readdir(dp)) != NULL)
					{
						if (entry->d_type == DT_REG || entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
						{
							trie.insert(entry->d_name);
						}
					}
					closedir(dp);
				}
			}
		}
	}
	int run()
	{
		while (true)
		{
			cout.flush();
			cerr.flush();
			cout << "$ ";
			string input;
			char ch;
			while (true)
			{
				ch = getchar();
				if (ch == '\n')
				{
					cout << endl;
					break;
				}
				else if (ch == 127 || ch == '\b')
				{
					if (!input.empty())
					{
						input.pop_back();
						cout << "\b \b";
					}
				}
				else if (ch == '\t')
				{
					// Extract the current word (including underscores) before the cursor
					size_t pos = input.find_last_of(" ");
					string prefix = (pos == string::npos) ? input : input.substr(pos + 1);

					// Now, prefix includes any underscores or other characters typed
					vector<string> matches = trie.complete(prefix);
					if (matches.size() == 1)
					{
						input += matches[0].substr(prefix.size()) + " ";
						cout << matches[0].substr(prefix.size()) << " ";
					}
					else if (matches.size() > 1)
					{
						string lcp = longestCommonPrefix(matches);
						if (lcp.size() > prefix.size())
						{
							input += lcp.substr(prefix.size());
							cout << lcp.substr(prefix.size());
						}
						cout << "\n";
						cout << '\a';
						for (const string &m : matches)
							cout << m << "  ";
						cout << "\n$ " << input;
					}
					else if (matches.empty())
					{
						cout << '\x07';
						cout.flush();
					}
				}
				else
				{
					input += ch;
					cout << ch;
				}
			}

			Parser parser;
			vector<Command> cmds = parser.parsePipe(input);
			if (cmds.size() > 1)
			{
				Executor executor;
				executor.executePipeline(cmds);
			}
			else
			{
				Executor executor;
				executor.execute(cmds[0]);
			}
		}
	}
};

// <--- Main --->
int main()
{
	cout << std::unitbuf;
	cerr << std::unitbuf;
	setRawMode(true);
	Shell shell;
	shell.run();
	setRawMode(false);
	return 0;
}

/* Doubts"""
	1. Converting vector<string> to char** for execvp
	2. Pipeline implementation for multiple commands not working...
	3. Handling built-in commands in pipeline
*/

/*
	Things To do:
	1. Handle empty input
	2. Using trie's for auto-completion
	7. Showing suggestions....
				3. Signal handling (Ctrl+C, Ctrl+D)
				4. More built-in commands (cd, ls, etc.)
				5. Error handling and edge cases
				6. Comments and documentation

*/
