#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

std::string toUpper(std::string text) {
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        }
    );
    return text;
}

bool getDnsType(const std::string& typeName, std::uint16_t& typeCode) {
    const std::string type = toUpper(typeName);

    if (type == "A") {
        typeCode = 1;
    } else if (type == "NS") {
        typeCode = 2;
    } else if (type == "CNAME") {
        typeCode = 5;
    } else if (type == "MX") {
        typeCode = 15;
    } else if (type == "AAAA") {
        typeCode = 28;
    } else {
        return false;
    }

    return true;
}

void appendUint16(
    std::vector<std::uint8_t>& message,
    std::uint16_t value
) {
    message.push_back(
        static_cast<std::uint8_t>((value >> 8) & 0xFF)
    );
    message.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

void encodeDomainName(
    const std::string& hostname,
    std::vector<std::uint8_t>& message
) {
    if (hostname.empty()) {
        throw std::runtime_error("Hostname cannot be empty.");
    }

    std::string name = hostname;

    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }

    if (name.empty() || name.size() > 253) {
        throw std::runtime_error("Invalid hostname length.");
    }

    std::size_t labelStart = 0;

    while (labelStart < name.size()) {
        std::size_t dotPosition = name.find('.', labelStart);

        if (dotPosition == std::string::npos) {
            dotPosition = name.size();
        }

        const std::size_t labelLength = dotPosition - labelStart;

        if (labelLength == 0 || labelLength > 63) {
            throw std::runtime_error("Invalid DNS label length.");
        }

        message.push_back(static_cast<std::uint8_t>(labelLength));

        for (std::size_t index = labelStart;
             index < dotPosition;
             ++index) {
            message.push_back(static_cast<std::uint8_t>(name[index]));
        }

        labelStart = dotPosition + 1;
    }

    message.push_back(0);
}

std::vector<std::uint8_t> buildDnsQuery(
    const std::string& hostname,
    std::uint16_t typeCode,
    std::uint16_t transactionId
) {
    std::vector<std::uint8_t> query;

    appendUint16(query, transactionId);
    appendUint16(query, 0x0100);
    appendUint16(query, 1);
    appendUint16(query, 0);
    appendUint16(query, 0);
    appendUint16(query, 0);

    encodeDomainName(hostname, query);
    appendUint16(query, typeCode);
    appendUint16(query, 1);

    return query;
}

void printUsage() {
    std::cerr << "Usage:\n";
    std::cerr << "  ./dnsclient --supported\n";
    std::cerr
        << "  ./dnsclient --query "
        << "<dns-server> <hostname> <type>\n";
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--supported") {
        std::cout << "A AAAA CNAME NS MX\n";
        return 0;
    }

    if (argc != 5 || std::string(argv[1]) != "--query") {
        printUsage();
        return 1;
    }

    const std::string dnsServer = argv[2];
    const std::string hostname = argv[3];
    const std::string typeName = argv[4];

    if (dnsServer.empty() || hostname.empty()) {
        std::cerr << "DNS server and hostname cannot be empty.\n";
        return 1;
    }

    std::uint16_t typeCode = 0;

    if (!getDnsType(typeName, typeCode)) {
        std::cerr << "Unsupported DNS record type: "
                  << typeName << '\n';
        return 1;
    }

    try {
        constexpr std::uint16_t transactionId = 0x1234;
        const std::vector<std::uint8_t> query = buildDnsQuery(
            hostname,
            typeCode,
            transactionId
        );

        static_cast<void>(dnsServer);

        std::cerr << "Constructed DNS query of "
                  << query.size() << " bytes.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
