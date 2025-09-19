#include <bits/stdc++.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>

using namespace std;

struct Command
{
	string name;
	vector<string> args;
};

// <--- Utilities ----> //
Command parseInput(const string input)
{
	Command cmd;
	istringstream inp(input);
	string token;
	if (inp >> token)
	{
		cmd.name = token;
	}
	while (inp >> token)
	{
		cmd.args.push_back(token);
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

bool isBuiltin(const string &name)
{
	return (name == "exit" || name == "echo" || name == "type" || name == "pwd");
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

// Helper: convert vector<string> to char* array for execvp
char **vec_to_cstr_array(const vector<string> &v)
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

			if (isBuiltin(commands[i].name))
			{
				if (commands[i].name == "echo")
					handleEcho(commands[i]);
				else if (commands[i].name == "pwd")
					handlePwd(commands[i]);
				else if (commands[i].name == "type")
					handleType(commands[i]);
				exit(0);
			}

			// External command
			vector<string> args;
			args.push_back(commands[i].name);
			args.insert(args.end(), commands[i].args.begin(), commands[i].args.end());
			char **argv = vec_to_cstr_array(args);
			execvp(argv[0], argv);
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
	if (cmd.name == "exit")
	{
		handleExit(cmd);
		cout.flush();
		cerr.flush();
	}
	else if (cmd.name == "echo")
		handleEcho(cmd);
	else if (cmd.name == "type")
		handleType(cmd);
	else if (cmd.name == "pwd")
		handlePwd(cmd);
	else if (cmd.name == "cd")
		handleCd(cmd);
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

// <--- Main --->
int main()
{
	cout << std::unitbuf;
	cerr << std::unitbuf;
	while (true)
	{
		cout.flush();
		cerr.flush();
		cout << "$ ";
		string input;
		if (!getline(cin, input))
			return 0;

		vector<Command> cmds = parsePipe(input);
		if (cmds.size() > 1)
		{
			executePipeline(cmds);
		}
		else
			execute(cmds[0]);
	}
	return 0;
}
