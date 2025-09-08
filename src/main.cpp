#include <iostream>
#include <string>
using namespace std;

int main() {  
  cout << std::unitbuf;
  cerr << std::unitbuf;
  cout << "$ ";
  string input;
  getline(std::cin, input);
  cout<< input << ": command not found"<< '\n';
  main();
}
