#include <iostream>
#include <set>
#include <string>

int main() {
    // Valid C++98 initialization
    std::set<std::string> mySet;
    
    // Elements must be added manually
    mySet.insert("Apple");
    mySet.insert("Banana");
    
    // Explicit iterator type required in C++98 (cannot use 'auto')
    for (std::set<std::string>::const_iterator it = mySet.begin(); it != mySet.end(); ++it) {
        std::cout << *it << std::endl;
    }
    
    return 0;
}