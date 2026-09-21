#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--supported") {
        std::cout << "A AAAA CNAME NS MX\n";
        return 0;
    }

    std::cerr << "Usage:\n";
    std::cerr << "  ./dnsclient --supported\n";
    std::cerr << "  ./dnsclient --query <dns-server> <hostname> <type>\n";

    return 1;
}