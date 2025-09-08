#include <iostream>
#include <string>
using namespace std;
struct searchResult{
  bool found;
  int index;
};

searchResult searchArray(string commands[], int size, string input){
  for(int i=0;i<size;i++){
    if(commands[i] == input) return {true, i};
  }
  return {false, -1};
}



int main() {  
  cout << std::unitbuf;
  cerr << std::unitbuf;
  cout << "$ ";
  string input;
  getline(std::cin, input);
  string commands[] = {"exit", "echo","type"};
  
  int commandsSize = sizeof(commands)/sizeof(string);
  int whitespaceIndex = input.find(" ");

  string inputPrefix = input.substr(0, whitespaceIndex);
  string inputSuffix = input.substr(whitespaceIndex+1, input.length());
  
  searchResult result = searchArray(commands, commandsSize, inputPrefix);
  bool flag = 0;
  if(result.found){
    if(commands[result.index] == "exit") return stoi(inputSuffix);
    else if(commands[result.index] == "echo"){
      cout<<inputSuffix<<'\n';
      flag = 1;
    }
  } 
  if(flag==0){
    cout<< input << ": command not found"<< '\n';
  }
  main();
}
