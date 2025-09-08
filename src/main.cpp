#include <iostream>
#include <string>
using namespace std;
struct searchResult{
  bool found;
  int index;
}

searchResult searchArray(string commands[], int size, string input){
  for(int i=0;i<size;i++){
    if(commands[i] == input) return {true, i};
  }
  return {false, -1};
}



int main() {  
  cout << std::unitbuf;
  cerr << std::unitbuf;
  string commands[] = {"exit", "echo","type"};
  int commandsSize = commands.size();
  int whitespaceIndex = input.find(" ");

  string inputPrefix = input.substr(0, whitespaceIndex);
  string inputSufix = input.substr(whitespaceIndex+1, input.length());
  
  searchResult result = searchArray(commands, commandsSize, inputPrefuix);
  if(result.found){
    if(commands[result.index] == "exit") return stoi(inputSufix);
  }
  
  cout << "$ ";
  string input;
  getline(std::cin, input);
  if(input == "exit")
  cout<< input << ": command not found"<< '\n';
  main();
}
