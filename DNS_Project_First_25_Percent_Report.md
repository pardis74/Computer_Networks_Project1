# DNS Client and Message Parser

## Progress Report: First 25 Percent

**Student:** Pardis Sadatian  
**Language:** C++17  
**Repository:** `pardis74/Computer_Networks_Project1`  
**Optional DNS record type:** MX

## 1. Project objective

The purpose of this project is to implement a DNS client and message parser without using a library that constructs or parses DNS messages. The final program will create standard DNS queries, transmit them to a DNS server over UDP port 53, receive raw DNS responses, validate those responses, parse supported resource records, and print machine-readable output.

The required record types are A, AAAA, CNAME, and NS. MX was selected as the additional optional type.

## 2. Project structure and build system

The project was created in:

```text
/Users/pardissadatian/Desktop/dns-project
```

The current structure includes:

```text
dns-project/
├── docker/
├── src/
│   └── main.cpp
├── .gitignore
├── DOCKER.md
├── Makefile
└── README.md
```

The Makefile compiles the program with `g++` using C++17 and the following compiler options:

```text
-std=c++17 -Wall -Wextra -Wpedantic -O2
```

The program is built with:

```bash
make
```

The required executable is produced as:

```text
./dnsclient
```

Generated binaries, object files, swap files, and macOS metadata are excluded through `.gitignore`.

## 3. Supported-types interface

The required supported-types mode has been implemented:

```bash
./dnsclient --supported
```

It produces:

```text
A AAAA CNAME NS MX
```

The four mandatory record types are printed first, followed by the selected optional MX type.

## 4. Command-line validation

The program recognizes query commands in this format:

```bash
./dnsclient --query <dns-server> <hostname> <type>
```

It verifies that query mode receives exactly the expected arguments. Invalid or incomplete commands cause usage information to be written to standard error. Diagnostic messages are kept separate from standard output so they will not interfere with the machine-readable output required by the autograder.

## 5. DNS type conversion

The program converts record-type names into the numeric codes required by the DNS wire format:

| Record type | Numeric code |
|---|---:|
| A | 1 |
| NS | 2 |
| CNAME | 5 |
| MX | 15 |
| AAAA | 28 |

Type names are handled without case sensitivity. For example, both `AAAA` and `aaaa` are recognized as type 28. Unsupported record types, such as TXT in this MX-based implementation, are rejected safely.

## 6. Network byte order

The `appendUint16` function stores 16-bit integers in network byte order. The high byte is added to the message first, followed by the low byte. This is necessary because the DNS wire format uses big-endian byte order for header fields, record types, classes, lengths, and other numeric values.

## 7. Domain-name encoding

The `encodeDomainName` function converts a normal hostname into DNS wire format. DNS does not transmit a domain name as a dot-separated string. Each label is preceded by a one-byte length, and the complete name ends with a zero byte.

For example:

```text
simple.lab.test
```

is encoded conceptually as:

```text
6 simple 3 lab 4 test 0
```

The encoder currently performs these validations:

- The hostname cannot be empty.
- A textual hostname cannot exceed 253 characters.
- Each label must contain between 1 and 63 bytes.
- Empty labels such as the middle label in `simple..lab.test` are rejected.
- A final period is accepted, so `simple.lab.test.` is treated as a valid fully qualified name.

## 8. DNS query construction

The `buildDnsQuery` function constructs the DNS header and question section as a byte vector. The current query contains a 12-byte DNS header with these values:

| Header field | Current value | Meaning |
|---|---:|---|
| Transaction ID | `0x1234` | Temporary fixed identifier for construction testing |
| Flags | `0x0100` | Standard query with recursion desired |
| QDCOUNT | 1 | One question |
| ANCOUNT | 0 | No answers in a query |
| NSCOUNT | 0 | No authority records |
| ARCOUNT | 0 | No additional records |

The question section contains:

- QNAME: the encoded hostname;
- QTYPE: the numeric record-type code; and
- QCLASS: 1, representing the Internet class.

For `simple.lab.test`, the full query is 33 bytes:

```text
12-byte header + 17-byte encoded name + 2-byte QTYPE + 2-byte QCLASS
```

## 9. Tests completed

The following commands were tested:

```bash
./dnsclient --supported
./dnsclient --query 10.53.0.53 simple.lab.test A
./dnsclient --query 10.53.0.53 simple.lab.test aaaa
./dnsclient --query 10.53.0.53 simple.lab.test. A
./dnsclient --query 10.53.0.53 simple..lab.test A
./dnsclient --query 10.53.0.53 example.lab.test TXT
```

The observed results were:

```text
A AAAA CNAME NS MX
Constructed DNS query of 33 bytes.
Constructed DNS query of 33 bytes.
Constructed DNS query of 33 bytes.
Invalid DNS label length.
Unsupported DNS record type: TXT
```

These results confirm that supported-types mode, case-insensitive type conversion, DNS name encoding, trailing-period handling, invalid-label detection, unsupported-type rejection, and basic query construction are working.

## 10. Git and repository workflow

Development has been divided into feature branches, including:

```text
feature/project-skeleton
feature/query-validation
feature/query-construction
```

The local project is connected to two remotes:

```text
origin  https://github.com/pardis74/Computer_Networks_Project1.git
starter https://github.com/UGA-Networks/dns-project.git
```

The `origin` remote is the personal project repository. The `starter` remote remains connected to the course starter repository for reference.

## 11. Current completion status

Approximately 25 percent of the project has been completed. The current implementation can validate query arguments, interpret the five selected DNS record types, encode domain names, and construct a correctly structured DNS query message in memory.

The current version does not yet send the query or parse a response.

## 12. Remaining work

The remaining stages include:

1. Create a UDP socket and send the constructed query to port 53.
2. Receive the raw DNS response with a timeout.
3. Validate the response transaction ID, response flag, counts, and message length.
4. Parse the DNS question section.
5. Decode uncompressed names and compression pointers safely.
6. Parse A, AAAA, CNAME, NS, and MX resource records.
7. Skip unsupported records using RDLENGTH.
8. Handle NOERROR, NXDOMAIN, and other response codes.
9. Produce the exact required machine-readable output.
10. Test all supplied Docker records and malformed or edge-case messages.

## 13. Conclusion

The first quarter of the DNS client project establishes the program structure and the core message-construction logic. The program builds successfully, exposes the required interface, validates record types and hostnames, represents numeric fields in network byte order, encodes DNS names, and constructs a complete DNS query in memory. The next milestone is real UDP communication with the supplied Docker DNS server.
