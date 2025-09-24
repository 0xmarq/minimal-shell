#Shell Project

## Features

### 1.Auto-Completion

### Implementation Notes

1. **Trie Data Structure:**
   - All built-in command names are stored in a Trie (prefix tree).
   - Each character of a command is a node in the Trie.

- The Trie is built at shell startup by inserting all built-in command names.
- The shell uses the Trie to find completions for the current input prefix.
- This can be extended to support external commands and file names.
- Using dfs to find all the possible prefixes and using a complete function to tab complete
- Using sort() for lexical ordering of prefixes

### Issues

    - Are you detecting Tab presses??
        - You have to detect /t literals
          setting terminal to raw mode.
        - Using getline() wont trigger
          auto-completion.

---

## Parsing Char-by-char;;

        -Instead of parsing using simple
            getline(), stringstream and
            tokens
        -Parsing char by char is needed to
        record events like tab press,
        Ctrl+C...
        - Setting terminal to Raw Mode

## Issues
