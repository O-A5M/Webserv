#include <iostream>
#include <string>

int main() {
    std::string text = "Hello World.py";
    std::size_t position = text.find(".py"); // Using the built-in function
    
    if (position != std::string::npos) {
        std::string extension = text.substr(position); // Extracting the extension
        std::cout << "Extension found: " << extension << std::endl;
    } else {
        std::cout << "Not found\n";
    }
}
