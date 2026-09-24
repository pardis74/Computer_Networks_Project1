# DNS Client and Message Parser

## Final Project Report

**Student:** Pardis Sadatian  
**Language:** C++17  
**Selected optional record type:** MX  
**Project status:** Complete

## 1. Project Overview

This project implements a DNS client and DNS message parser from scratch in C++. The program constructs a DNS query using the standard DNS wire format, sends it to a DNS server over UDP port 53, receives the raw response, validates the response, parses supported resource records, and prints machine-readable results.

No DNS construction or parsing libraries are used. The implementation relies only on standard C++ facilities, POSIX socket APIs, networking headers, and `inet_ntop()` for address formatting.

## 2. Required Interface

The project builds with:

```bash
make
```

The build produces:

```text
./dnsclient
```

Supported-types mode:

```bash
./dnsclient --supported
```

Output:

```text
A AAAA CNAME NS MX
```

Query mode:

```bash
./dnsclient --query <dns-server> <hostname> <type>
```

## 3. Project Structure

```text
Makefile
src/main.cpp
src/mainpart2.cpp
src/mainpart3.cpp
docker/
README.md
DOCKER.md
```

The staged source files preserve the development history:

- `main.cpp` is the final entry point used by the Makefile.
- `mainpart2.cpp` contains query construction, UDP transport, response validation, compression decoding, and A/AAAA parsing.
- `mainpart3.cpp` extends the parser with CNAME, NS, and MX support.

## 4. DNS Query Construction

The client constructs the DNS message header directly in network-byte order. Each query contains:

- A randomly generated 16-bit transaction ID
- The recursion-desired flag
- One question
- Zero answer, authority, and additional records
- The hostname encoded as DNS labels
- The numeric query type
- Internet class (`IN`)

Hostname validation rejects empty labels, labels longer than 63 bytes, and names longer than the DNS name limit. A trailing dot is accepted and removed before encoding.

## 5. UDP Networking

The program creates an IPv4 UDP socket and sends the constructed query to port 53 of the requested DNS server. It uses:

- `socket()`
- `inet_pton()`
- `sendto()`
- `recvfrom()`
- `setsockopt()`
- `close()`

A three-second receive timeout prevents the program from waiting indefinitely when a server does not respond. Networking errors are printed to standard error.

## 6. DNS Response Validation

Before interpreting resource records, the client verifies:

- The message contains the complete 12-byte DNS header.
- The response transaction ID matches the query transaction ID.
- The QR flag identifies the packet as a response.
- The response is not marked as truncated.
- The response contains a question.
- The returned question type and class match the original query.
- All fixed-width fields and RDATA fields are within the received message.

Common DNS response codes are converted to machine-readable status names, including `NOERROR`, `FORMERR`, `SERVFAIL`, `NXDOMAIN`, `NOTIMP`, and `REFUSED`.

## 7. DNS Name Encoding and Compression

Query names are encoded as length-prefixed labels followed by a zero byte. Response names are decoded using both ordinary labels and DNS compression pointers.

The decompression logic:

- Detects pointers using the two highest bits of a label byte
- Validates pointer offsets
- Preserves the correct location of the next field after following a pointer
- Limits pointer traversal to prevent compression-pointer loops
- Performs boundary checks before reading labels

The same decoder is used for owner names and name-based RDATA values.

## 8. Supported Resource Records

### 8.1 A

An A record must contain four RDATA bytes. The address is converted to readable IPv4 form with `inet_ntop()`.

```text
ANSWER simple.lab.test A TTL=60 VALUE=192.0.2.10
```

### 8.2 AAAA

An AAAA record must contain 16 RDATA bytes. The address is converted to readable IPv6 form.

```text
ANSWER ipv6.lab.test AAAA TTL=90 VALUE=2001:db8::10
```

### 8.3 CNAME

CNAME RDATA is decoded as a compression-aware domain name.

```text
ANSWER alias.lab.test CNAME TTL=120 VALUE=simple.lab.test
```

### 8.4 NS

NS RDATA is decoded as a compression-aware authoritative server name. Multiple NS answers are printed on separate lines.

```text
ANSWER lab.test NS TTL=300 VALUE=ns1.lab.test
ANSWER lab.test NS TTL=300 VALUE=ns2.lab.test
```

### 8.5 MX

MX was selected as the optional type. Its RDATA contains a 16-bit preference followed by a compression-aware exchange name.

```text
ANSWER mail.lab.test MX TTL=300 PREFERENCE=10 EXCHANGE=mx1.lab.test
```

## 9. Unsupported Record Handling

The parser does not assume that every answer has a supported type. After reading a resource-record header, it validates `RDLENGTH` and advances by exactly that number of bytes when the type is unsupported. This permits parsing of later records without interpreting unknown RDATA structures.

## 10. Output Behavior

Successful queries print a status line, a question line, and zero or more answer lines:

```text
STATUS NOERROR
QUESTION simple.lab.test A
ANSWER simple.lab.test A TTL=60 VALUE=192.0.2.10
```

An undefined name prints:

```text
STATUS NXDOMAIN
QUESTION missing.lab.test A
```

All normal results are written to standard output. Usage information, validation failures, timeouts, and networking errors are written to standard error.

## 11. Docker Testing

The project was built inside the supplied GCC student container to ensure compatibility with the grading environment:

```bash
make clean
make
./dnsclient --supported
```

The resulting Linux executable was confirmed to exist and run successfully. The supplied BIND server reported a healthy status before live queries were performed.

The following test categories were exercised:

| Test | Purpose |
|---|---|
| `simple.lab.test A` | IPv4 response |
| `ipv6.lab.test AAAA` | IPv6 response |
| `alias.lab.test CNAME` | Name-based RDATA and compression |
| `lab.test NS` | Multiple NS records |
| `mail.lab.test MX` | Optional MX fields |
| `multi.lab.test A` | Multiple-answer traversal |
| `missing.lab.test A` | NXDOMAIN handling |

The multiple-A query successfully returned all three records. Their order differed from the zone-file order, which is valid because DNS answer ordering is not guaranteed.

Verified examples included:

```text
STATUS NOERROR
QUESTION multi.lab.test A
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.23
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.22
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.21
```

```text
STATUS NXDOMAIN
QUESTION missing.lab.test A
```

## 12. Problems Encountered and Resolved

### BIND configuration permissions

The DNS container initially became unhealthy because BIND could not read the mounted configuration and zone files. Read permissions were corrected, and the container subsequently reported a healthy status.

### Apple Silicon platform warning

Docker reported that the BIND image used `linux/amd64` while the computer used `arm64`. Docker Desktop successfully handled the image through emulation, so the warning did not prevent testing.

### Executable format error

A binary compiled on macOS produced an `Exec format error` inside the Linux container. The binary was removed and rebuilt inside the student container, producing the correct Linux executable.

### Joined terminal commands

Several terminal commands were accidentally pasted together. Running each command separately resolved the issue and allowed clean compilation and testing.

## 13. Git and GitHub Workflow

Development was divided into incremental branches and milestones, including:

- Project skeleton and supported types
- Query validation and construction
- UDP response parsing
- CNAME, NS, and MX parsing
- Final integration

The final implementation was committed with:

```text
Complete DNS client implementation
```

The final branch is:

```text
feature/final-integration
```

The project repository is:

```text
https://github.com/pardis74/Computer_Networks_Project1
```

## 14. Final Submission Checklist

- [x] Builds non-interactively with `make`
- [x] Produces `./dnsclient`
- [x] Implements `--supported`
- [x] Constructs DNS queries directly
- [x] Sends and receives UDP DNS traffic
- [x] Validates DNS response fields
- [x] Decodes compressed domain names
- [x] Parses A records
- [x] Parses AAAA records
- [x] Parses CNAME records
- [x] Parses NS records
- [x] Parses MX records
- [x] Handles multiple answers
- [x] Handles NXDOMAIN
- [x] Skips unsupported records safely
- [x] Writes diagnostics to standard error
- [x] Tested in the supplied Docker environment
- [x] Final implementation committed and pushed
- [ ] Merge the final pull request into `main`
- [ ] Confirm a clean clone builds successfully
- [ ] Export `git log` for the eLC submission archive

## 15. Conclusion

The completed program implements the required DNS client and message parser without relying on DNS helper libraries. It builds valid wire-format queries, communicates with a DNS server over UDP, safely parses compressed responses, supports all mandatory types plus MX, handles multiple answers and NXDOMAIN, and produces machine-readable output suitable for automated grading.
