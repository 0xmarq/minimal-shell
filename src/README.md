# Minimal Unix Shell

## Setup

1. Clone this repository using \
   `git clone https://github.com/0xmarq/minimal-shell`
2. Make sure your Cmake build points to `CMAKE_CXX_STANDARD 23`
   if its different, you'll be prompted an error regarding this once you run the program

3. Run the program by executing `your_program.sh`, present in the root dir
4. The main file is under the `\src` folder

## Implementation Notes

### 1.Auto-Completion

---

`1.1 Trie Data Structure`

    All built-in command names are stored in a Trie (prefix tree).
    Each character of a command is a node in the Trie.

    - The Trie is built at shell startup by inserting all built-in command names.
    - The shell uses the Trie to find completions for the current input prefix.
    - This can be extended to support external commands and file names.
    - Using dfs to find all the possible prefixes and using a complete function to tab complete
    - Using sort() for lexical ordering of prefixes

### Issues

    - Are you detecting Tab presses??
        - You have to detect /t literals by
        setting terminal to raw mode.
        - Using getline() wont trigger
         auto-completion.
    - Path-based executable completion is partially implemented and requires refactoring to correctly handle PATH resolution and edge cases.

---

## 2.Parsing Char-by-char

        -Instead of parsing using simple
         getline(), stringstream and tokens
        -Parsing char by char is needed to
          record events like tab press, Ctrl+C...
        -Setting terminal to Raw Mode

`This actually happens by just taking every character as we type and comparing if it equals some trigger. If yes, do the process else add the char to the input`

## Issues

    - This is humbling, smh

## 3.Termios

    - Setting terminal into Raw mode
        -toggling attributes of the termios object , ICANON etc...

## 4.Handling quoting

### Basic Behaviour of shell

    - Inside double quotes, \ should escape special characters but not turn \n into a newline.
    It should literally insert n.
    example-

`$ echo "foo\nbar"`\
 `foo\nbar`

    - Inside single quotes, everything is literal, the only exception is to escape a single ' is by opening and closing it,

` ''\'97' becomes '97 .` \
` 'foo '\''bar' becomes foo 'bar.`

    - Outside quotes, \ escapes the very next character literally
        so, \n becomes a new line, '\' escapes a space.
    - Tokenizer handles quoting logic including backslashes and escapes.

## 5. Pipes and parsing

`-char **vec_to_cstr_array(...)-` why this exists.

execvp wants: `char *argv[] = { "ls", "-l", NULL };`

    so converting vector<string> to char**
    setvbuf(stdout, NULL, _IONBF, 0);
        - What problem this solves
        - By default:
            - stdout is line-buffered when connected to a terminal
            - fully buffered when connected to a pipe
        - That means:
            - output may sit in buffer
            - downstream process waits
            - pipeline output appears delayed or reordered

    - multi-command pipelines using POSIX pipe, fork, and dup2.
    Each command runs in its own process, with stdout of command i redirected to stdin of command i+1.
    - Pipe file descriptors are carefully closed in both parent and child processes to avoid deadlocks.
    - Output buffering is disabled in pipeline children using setvbuf to ensure       correct streaming behavior, especially for built-in commands executed without exec.
    messy plumbing.

## 6. History persistence

    - simply using a vector<string> to record all the strings
    - not optimal, can use ring buffer of upto last 100 inputs
    - Limited history handling, fetching the last n inputs, etc.
    - retrieving commands on using the upward arrow.

    History Navigation
    - Interactive command history recall using terminal raw mode.
    - Arrow key escape sequences are detected during char-by-char input, allowing users to navigate previously executed commands with the up and down arrows.
    - The input line is dynamically redrawn without re-executing commands until confirmed by Enter.

# Splitting the project into seperate files.

    src/
    ├── main.cpp
    ├── shell.h/cpp
    ├── trie.h/cpp
    ├── parser.h/cpp
    ├── executor.h/cpp
    ├── builtin.h/cpp
