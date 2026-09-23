#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr uint16_t DNS_CLASS_IN = 1;
constexpr uint16_t DNS_TYPE_A = 1;
constexpr uint16_t DNS_TYPE_NS = 2;
constexpr uint16_t DNS_TYPE_CNAME = 5;
constexpr uint16_t DNS_TYPE_MX = 15;
constexpr uint16_t DNS_TYPE_AAAA = 28;
constexpr std::size_t DNS_HEADER_SIZE = 12;

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::toupper(character));
                   });
    return value;
}

uint16_t getDnsType(const std::string& typeText) {
    const std::string type = toUpper(typeText);
    if (type == "A") return DNS_TYPE_A;
    if (type == "AAAA") return DNS_TYPE_AAAA;
    if (type == "CNAME") return DNS_TYPE_CNAME;
    if (type == "NS") return DNS_TYPE_NS;
    if (type == "MX") return DNS_TYPE_MX;
    throw std::runtime_error("Unsupported DNS record type: " + typeText);
}

std::string getTypeName(uint16_t type) {
    switch (type) {
        case DNS_TYPE_A: return "A";
        case DNS_TYPE_AAAA: return "AAAA";
        case DNS_TYPE_CNAME: return "CNAME";
        case DNS_TYPE_NS: return "NS";
        case DNS_TYPE_MX: return "MX";
        default: return "TYPE" + std::to_string(type);
    }
}

void appendUint16(std::vector<uint8_t>& message, uint16_t value) {
    message.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    message.push_back(static_cast<uint8_t>(value & 0xff));
}

void encodeDomainName(const std::string& hostname,
                      std::vector<uint8_t>& message) {
    std::string name = hostname;
    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }
    if (name.empty() || name.size() > 253) {
        throw std::runtime_error("Invalid DNS hostname length.");
    }

    std::size_t labelStart = 0;
    while (labelStart < name.size()) {
        const std::size_t dot = name.find('.', labelStart);
        const std::size_t labelEnd =
            (dot == std::string::npos) ? name.size() : dot;
        const std::size_t labelLength = labelEnd - labelStart;

        if (labelLength == 0 || labelLength > 63) {
            throw std::runtime_error("Invalid DNS label length.");
        }

        message.push_back(static_cast<uint8_t>(labelLength));
        message.insert(message.end(), name.begin() + labelStart,
                       name.begin() + labelEnd);

        if (dot == std::string::npos) break;
        labelStart = dot + 1;
    }
    message.push_back(0);
}

uint16_t generateTransactionId() {
    std::random_device randomDevice;
    std::uniform_int_distribution<unsigned int> distribution(0, 65535);
    return static_cast<uint16_t>(distribution(randomDevice));
}

std::vector<uint8_t> buildDnsQuery(const std::string& hostname,
                                   uint16_t queryType,
                                   uint16_t transactionId) {
    std::vector<uint8_t> query;
    query.reserve(512);

    appendUint16(query, transactionId);
    appendUint16(query, 0x0100);  // Recursion desired
    appendUint16(query, 1);       // One question
    appendUint16(query, 0);
    appendUint16(query, 0);
    appendUint16(query, 0);
    encodeDomainName(hostname, query);
    appendUint16(query, queryType);
    appendUint16(query, DNS_CLASS_IN);
    return query;
}

uint16_t readUint16(const std::vector<uint8_t>& message,
                    std::size_t& offset) {
    if (offset + 2 > message.size()) {
        throw std::runtime_error("Truncated DNS message.");
    }
    const uint16_t value =
        static_cast<uint16_t>((static_cast<uint16_t>(message[offset]) << 8) |
                              message[offset + 1]);
    offset += 2;
    return value;
}

uint32_t readUint32(const std::vector<uint8_t>& message,
                    std::size_t& offset) {
    if (offset + 4 > message.size()) {
        throw std::runtime_error("Truncated DNS message.");
    }
    const uint32_t value =
        (static_cast<uint32_t>(message[offset]) << 24) |
        (static_cast<uint32_t>(message[offset + 1]) << 16) |
        (static_cast<uint32_t>(message[offset + 2]) << 8) |
        static_cast<uint32_t>(message[offset + 3]);
    offset += 4;
    return value;
}

std::string decodeDomainName(const std::vector<uint8_t>& message,
                             std::size_t& offset) {
    std::string name;
    std::size_t cursor = offset;
    bool followedPointer = false;
    std::size_t pointerCount = 0;

    while (true) {
        if (cursor >= message.size()) {
            throw std::runtime_error("Truncated DNS name.");
        }

        const uint8_t length = message[cursor];
        if ((length & 0xc0) == 0xc0) {
            if (cursor + 1 >= message.size()) {
                throw std::runtime_error("Truncated DNS compression pointer.");
            }
            const std::size_t pointer =
                (static_cast<std::size_t>(length & 0x3f) << 8) |
                message[cursor + 1];
            if (pointer >= message.size()) {
                throw std::runtime_error("Invalid DNS compression pointer.");
            }
            if (!followedPointer) {
                offset = cursor + 2;
                followedPointer = true;
            }
            cursor = pointer;
            if (++pointerCount > message.size()) {
                throw std::runtime_error("DNS compression pointer loop.");
            }
            continue;
        }

        if ((length & 0xc0) != 0) {
            throw std::runtime_error("Invalid DNS label encoding.");
        }

        ++cursor;
        if (length == 0) {
            if (!followedPointer) offset = cursor;
            return name;
        }

        if (length > 63 || cursor + length > message.size()) {
            throw std::runtime_error("Invalid or truncated DNS label.");
        }
        if (!name.empty()) name.push_back('.');
        name.append(reinterpret_cast<const char*>(&message[cursor]), length);
        cursor += length;
    }
}

std::vector<uint8_t> sendDnsQuery(const std::string& server,
                                  const std::vector<uint8_t>& query) {
    const int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        throw std::runtime_error("Could not create UDP socket: " +
                                 std::string(std::strerror(errno)));
    }

    try {
        timeval timeout{};
        timeout.tv_sec = 3;
        if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout)) < 0) {
            throw std::runtime_error("Could not set socket timeout: " +
                                     std::string(std::strerror(errno)));
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(53);
        if (inet_pton(AF_INET, server.c_str(), &serverAddress.sin_addr) != 1) {
            throw std::runtime_error("Invalid IPv4 DNS server address: " + server);
        }

        const ssize_t sent =
            sendto(socketFd, query.data(), query.size(), 0,
                   reinterpret_cast<const sockaddr*>(&serverAddress),
                   sizeof(serverAddress));
        if (sent < 0 || static_cast<std::size_t>(sent) != query.size()) {
            throw std::runtime_error("Could not send DNS query: " +
                                     std::string(std::strerror(errno)));
        }

        std::array<uint8_t, 4096> buffer{};
        const ssize_t received = recvfrom(socketFd, buffer.data(), buffer.size(),
                                          0, nullptr, nullptr);
        if (received < 0) {
            throw std::runtime_error("Could not receive DNS response: " +
                                     std::string(std::strerror(errno)));
        }

        close(socketFd);
        return std::vector<uint8_t>(buffer.begin(), buffer.begin() + received);
    } catch (...) {
        close(socketFd);
        throw;
    }
}

std::string getStatusName(uint16_t responseCode) {
    switch (responseCode) {
        case 0: return "NOERROR";
        case 1: return "FORMERR";
        case 2: return "SERVFAIL";
        case 3: return "NXDOMAIN";
        case 4: return "NOTIMP";
        case 5: return "REFUSED";
        default: return "RCODE" + std::to_string(responseCode);
    }
}

void printAddressAnswer(const std::vector<uint8_t>& response,
                        const std::string& owner,
                        uint16_t type,
                        uint32_t ttl,
                        std::size_t rdataOffset,
                        uint16_t rdataLength) {
    int addressFamily = 0;
    if (type == DNS_TYPE_A && rdataLength == 4) {
        addressFamily = AF_INET;
    } else if (type == DNS_TYPE_AAAA && rdataLength == 16) {
        addressFamily = AF_INET6;
    } else {
        return;
    }

    std::array<char, INET6_ADDRSTRLEN> addressText{};
    if (inet_ntop(addressFamily, response.data() + rdataOffset,
                  addressText.data(), addressText.size()) == nullptr) {
        throw std::runtime_error("Could not format DNS address record.");
    }

    std::cout << "ANSWER " << owner << ' ' << getTypeName(type)
              << " TTL=" << ttl << " VALUE=" << addressText.data() << '\n';
}

void parseDnsResponse(const std::vector<uint8_t>& response,
                      uint16_t expectedTransactionId,
                      uint16_t expectedType) {
    if (response.size() < DNS_HEADER_SIZE) {
        throw std::runtime_error("DNS response is shorter than its header.");
    }

    std::size_t offset = 0;
    const uint16_t transactionId = readUint16(response, offset);
    const uint16_t flags = readUint16(response, offset);
    const uint16_t questionCount = readUint16(response, offset);
    const uint16_t answerCount = readUint16(response, offset);
    (void)readUint16(response, offset);
    (void)readUint16(response, offset);

    if (transactionId != expectedTransactionId) {
        throw std::runtime_error("DNS response transaction ID does not match.");
    }
    if ((flags & 0x8000) == 0) {
        throw std::runtime_error("Received packet is not a DNS response.");
    }
    if ((flags & 0x0200) != 0) {
        throw std::runtime_error("DNS response was truncated; TCP is not supported.");
    }
    if (questionCount == 0) {
        throw std::runtime_error("DNS response contains no question.");
    }

    std::string questionName;
    uint16_t questionType = 0;
    for (uint16_t index = 0; index < questionCount; ++index) {
        const std::string currentName = decodeDomainName(response, offset);
        const uint16_t currentType = readUint16(response, offset);
        const uint16_t currentClass = readUint16(response, offset);
        if (index == 0) {
            questionName = currentName;
            questionType = currentType;
            if (currentClass != DNS_CLASS_IN || questionType != expectedType) {
                throw std::runtime_error("DNS response question does not match query.");
            }
        }
    }

    const uint16_t responseCode = flags & 0x000f;
    std::cout << "STATUS " << getStatusName(responseCode) << '\n';
    std::cout << "QUESTION " << questionName << ' '
              << getTypeName(questionType) << '\n';

    if (responseCode != 0) return;

    for (uint16_t index = 0; index < answerCount; ++index) {
        const std::string owner = decodeDomainName(response, offset);
        const uint16_t type = readUint16(response, offset);
        const uint16_t recordClass = readUint16(response, offset);
        const uint32_t ttl = readUint32(response, offset);
        const uint16_t rdataLength = readUint16(response, offset);
        const std::size_t rdataOffset = offset;

        if (rdataOffset + rdataLength > response.size()) {
            throw std::runtime_error("Truncated DNS resource record.");
        }
        if (recordClass == DNS_CLASS_IN) {
            printAddressAnswer(response, owner, type, ttl, rdataOffset,
                               rdataLength);
        }

        // Unsupported records are safely skipped using RDLENGTH.
        offset = rdataOffset + rdataLength;
    }
}

void printUsage(const char* programName) {
    std::cerr << "Usage:\n"
              << "  " << programName << " --supported\n"
              << "  " << programName
              << " --query <dns-server> <hostname> <type>\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--supported") {
        std::cout << "A AAAA CNAME NS MX\n";
        return 0;
    }

    if (argc != 5 || std::string(argv[1]) != "--query") {
        printUsage(argv[0]);
        return 1;
    }

    try {
        const std::string server = argv[2];
        const std::string hostname = argv[3];
        const uint16_t queryType = getDnsType(argv[4]);
        const uint16_t transactionId = generateTransactionId();
        const std::vector<uint8_t> query =
            buildDnsQuery(hostname, queryType, transactionId);
        const std::vector<uint8_t> response = sendDnsQuery(server, query);
        parseDnsResponse(response, transactionId, queryType);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
