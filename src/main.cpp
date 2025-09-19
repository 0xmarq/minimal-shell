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
	if (commands.size() != 2)
	{
		cerr << "Only two-command pipelines are supported in this mode.\n";
		return;
	}
	vector<string> left;
	left.push_back(commands[0].name);
	left.insert(left.end(), commands[0].args.begin(), commands[0].args.end());
	vector<string> right;
	right.push_back(commands[1].name);
	right.insert(right.end(), commands[1].args.begin(), commands[1].args.end());

	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return;
	}

	pid_t pid1 = fork();
	if (pid1 == 0)
	{
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		// Builtin support for left command
		if (isBuiltin(commands[0].name))
		{
			if (commands[0].name == "echo")
				handleEcho(commands[0]);
			else if (commands[0].name == "pwd")
				handlePwd(commands[0]);
			else if (commands[0].name == "type")
				handleType(commands[0]);
			exit(0);
		}
		char **argv1 = vec_to_cstr_array(left);
		execvp(argv1[0], argv1);
		perror("execvp left");
		exit(EXIT_FAILURE);
	}

	pid_t pid2 = fork();
	if (pid2 == 0)
	{
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[1]);
		close(pipefd[0]);
		// Builtin support for right command
		if (isBuiltin(commands[1].name))
		{
			if (commands[1].name == "echo")
				handleEcho(commands[1]);
			else if (commands[1].name == "pwd")
				handlePwd(commands[1]);
			else if (commands[1].name == "type")
				handleType(commands[1]);
			exit(0);
		}
		char **argv2 = vec_to_cstr_array(right);
		execvp(argv2[0], argv2);
		perror("execvp right");
		exit(EXIT_FAILURE);
	}

	close(pipefd[0]);
	close(pipefd[1]);
	waitpid(pid1, nullptr, 0);
	waitpid(pid2, nullptr, 0);
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
