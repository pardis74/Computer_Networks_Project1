// Part 3 extends the tested UDP and basic parsing implementation from Part 2.
// The macros give the included Part 2 entry point and parser private names so
// this file can provide the expanded parser and its own main function.
#define main mainPart2
#define parseDnsResponse parseDnsResponsePart2
#include "mainpart2.cpp"
#undef parseDnsResponse
#undef main

namespace {

void verifyRdataConsumed(std::size_t parsedOffset, std::size_t rdataEnd) {
    if (parsedOffset != rdataEnd) {
        throw std::runtime_error("Invalid DNS resource-record length.");
    }
}

void printDomainNameAnswer(const std::vector<uint8_t>& response,
                           const std::string& owner,
                           uint16_t type,
                           uint32_t ttl,
                           std::size_t rdataOffset,
                           std::size_t rdataEnd) {
    std::size_t nameOffset = rdataOffset;
    const std::string value = decodeDomainName(response, nameOffset);
    verifyRdataConsumed(nameOffset, rdataEnd);

    std::cout << "ANSWER " << owner << ' ' << getTypeName(type)
              << " TTL=" << ttl << " VALUE=" << value << '\n';
}

void printMxAnswer(const std::vector<uint8_t>& response,
                   const std::string& owner,
                   uint32_t ttl,
                   std::size_t rdataOffset,
                   std::size_t rdataEnd) {
    if (rdataEnd - rdataOffset < 3) {
        throw std::runtime_error("Invalid MX resource record.");
    }

    std::size_t mxOffset = rdataOffset;
    const uint16_t preference = readUint16(response, mxOffset);
    const std::string exchange = decodeDomainName(response, mxOffset);
    verifyRdataConsumed(mxOffset, rdataEnd);

    std::cout << "ANSWER " << owner << " MX"
              << " TTL=" << ttl
              << " PREFERENCE=" << preference
              << " EXCHANGE=" << exchange << '\n';
}

void printResourceRecord(const std::vector<uint8_t>& response,
                         const std::string& owner,
                         uint16_t type,
                         uint32_t ttl,
                         std::size_t rdataOffset,
                         uint16_t rdataLength) {
    const std::size_t rdataEnd = rdataOffset + rdataLength;

    if (type == DNS_TYPE_A || type == DNS_TYPE_AAAA) {
        printAddressAnswer(response, owner, type, ttl, rdataOffset,
                           rdataLength);
    } else if (type == DNS_TYPE_CNAME || type == DNS_TYPE_NS) {
        printDomainNameAnswer(response, owner, type, ttl, rdataOffset,
                              rdataEnd);
    } else if (type == DNS_TYPE_MX) {
        printMxAnswer(response, owner, ttl, rdataOffset, rdataEnd);
    }
}

void parseDnsResponsePart3(const std::vector<uint8_t>& response,
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
    (void)readUint16(response, offset);  // Authority count
    (void)readUint16(response, offset);  // Additional count

    if (transactionId != expectedTransactionId) {
        throw std::runtime_error("DNS response transaction ID does not match.");
    }
    if ((flags & 0x8000) == 0) {
        throw std::runtime_error("Received packet is not a DNS response.");
    }
    if ((flags & 0x0200) != 0) {
        throw std::runtime_error(
            "DNS response was truncated; TCP is not supported.");
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
                throw std::runtime_error(
                    "DNS response question does not match query.");
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
            printResourceRecord(response, owner, type, ttl, rdataOffset,
                                rdataLength);
        }

        // Unknown record types remain safe to skip using RDLENGTH.
        offset = rdataOffset + rdataLength;
    }
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
        parseDnsResponsePart3(response, transactionId, queryType);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
