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

### Issues

    - Are you detecting Tab presses??
        - You have to detect /t literals, using readline library, parser.
        - Using getline() wont trigger auto-completion.

---

## To Do

- Add auto-completion for external commands and files.
- Improve interactive input handling (detect Tab key).
- Show suggestions in a user-friendly way.

## Parsing Char-by-char;;

    -Instead of parsing using simple getline(), stringstream and tokens
    -Parsing char by char is needed to record events like tab press, Ctrl+C...
    - Setting terminal to Raw Mode

## Issues

    - 1.When you autocomplete ech to echo and then type hello, your input buffer becomes:
    `echohello`
    because of auto-completion logic
    ```input += matches[0].substr(pf.size());
        cout<<matches[0].substr(pf.size())<<" ";```
