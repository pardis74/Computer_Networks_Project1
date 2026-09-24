# DNS Client and Message Parser

## Second 25% Progress Report

**Student:** Pardis Sadatian  
**Project:** DNS Client and Message Parser  
**Optional record type:** MX  
**Progress covered:** Approximately 25%–50% of the project

## 1. Objective

The objective of this stage was to extend the DNS query-construction work from the first milestone into a functional UDP DNS client. The program now sends a DNS query to a server, receives the raw response, validates important response fields, and parses basic IPv4 and IPv6 answer records.

The implementation continues to construct and interpret DNS messages directly without using a DNS client or DNS parsing library.

## 2. Source File

The implementation for this milestone was placed in:

```text
src/mainpart2.cpp
```

The original `src/main.cpp` was preserved so that the first milestone remains available for comparison.

## 3. Features Implemented

### 3.1 UDP communication

The program creates an IPv4 UDP socket and sends the constructed DNS query to port 53 of the supplied DNS server.

The implementation uses standard networking functions including:

- `socket()`
- `inet_pton()`
- `sendto()`
- `recvfrom()`
- `setsockopt()`
- `close()`

A three-second receive timeout prevents the client from waiting indefinitely when the server is unavailable.

### 3.2 DNS header parsing

The program reads the DNS response header in network-byte order and extracts:

- Transaction ID
- Flags
- Question count
- Answer count
- Authority-record count
- Additional-record count

Helper functions safely read 16-bit and 32-bit unsigned values while checking message boundaries.

### 3.3 Response validation

The parser performs several checks before interpreting answer records:

- The message must contain the complete 12-byte DNS header.
- The response transaction ID must match the query transaction ID.
- The QR flag must identify the packet as a response.
- The response must contain at least one question.
- The returned question type and class must match the query.
- A truncated UDP response is rejected because TCP fallback is outside the project scope.

Errors and diagnostic messages are printed to standard error.

### 3.4 DNS name decompression

The `decodeDomainName` function supports both ordinary DNS labels and compressed domain names.

Compression pointers are detected using the two highest bits of a length byte. The parser follows the pointer while preserving the correct position for the next field in the original message. It also checks pointer offsets and limits pointer traversal to protect against invalid pointer loops.

### 3.5 Status-code handling

The program converts common DNS response codes into machine-readable status names:

- `NOERROR`
- `FORMERR`
- `SERVFAIL`
- `NXDOMAIN`
- `NOTIMP`
- `REFUSED`

For an undefined name, the program prints the status and question without printing an answer.

### 3.6 A record parsing

For an `A` resource record, the parser verifies that `RDLENGTH` is four bytes and uses `inet_ntop()` to format the binary IPv4 address.

Example output:

```text
STATUS NOERROR
QUESTION simple.lab.test A
ANSWER simple.lab.test A TTL=60 VALUE=192.0.2.10
```

### 3.7 AAAA record parsing

For an `AAAA` resource record, the parser verifies that `RDLENGTH` is 16 bytes and formats the binary IPv6 address with `inet_ntop()`.

Example output:

```text
STATUS NOERROR
QUESTION ipv6.lab.test AAAA
ANSWER ipv6.lab.test AAAA TTL=90 VALUE=2001:db8::10
```

### 3.8 Multiple-answer traversal

The parser uses the answer count from the DNS header and processes each answer independently. Testing `multi.lab.test` confirmed that the program can print multiple resource records from one DNS response.

Example:

```text
STATUS NOERROR
QUESTION multi.lab.test A
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.21
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.22
ANSWER multi.lab.test A TTL=45 VALUE=192.0.2.23
```

### 3.9 Safe skipping of unsupported records

After reading a resource-record header, the parser checks `RDLENGTH` against the remaining message size. Records that are not yet interpreted are skipped by advancing exactly `RDLENGTH` bytes. This allows parsing to continue safely without assuming the structure of an unsupported record.

## 4. Docker and BIND Troubleshooting

The supplied BIND container initially reported an unhealthy status because it could not read the mounted configuration and zone files. The permissions were corrected with:

```bash
chmod 644 docker/bind/named.conf
chmod 644 docker/bind/db.lab.test
```

The environment was then recreated:

```bash
docker compose -f docker/docker-compose.yml down --remove-orphans
docker compose -f docker/docker-compose.yml up -d dns
docker compose -f docker/docker-compose.yml ps
```

The resolver subsequently reported a healthy status.

The Apple Silicon platform warning indicated that the BIND image was built for `linux/amd64` while the computer uses `arm64`. Docker Desktop successfully handled this through emulation, so the warning did not prevent testing.

## 5. Compilation Issue and Resolution

A binary compiled in the macOS terminal could not execute inside the Linux student container and produced an `Exec format error`. This occurred because macOS and Linux use different executable formats.

The generated binary was removed and rebuilt inside the student container:

```bash
rm -f dnsclient-part2
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 src/mainpart2.cpp -o dnsclient-part2
```

This produced a Linux executable that ran correctly inside Docker.

## 6. Tests Performed

The following commands were executed inside the student container:

```bash
./dnsclient-part2 --supported
./dnsclient-part2 --query 10.53.0.53 simple.lab.test A
./dnsclient-part2 --query 10.53.0.53 ipv6.lab.test AAAA
./dnsclient-part2 --query 10.53.0.53 multi.lab.test A
./dnsclient-part2 --query 10.53.0.53 missing.lab.test A
```

The observed results matched the supplied zone data:

| Test | Result |
|---|---|
| Supported types | `A AAAA CNAME NS MX` |
| `simple.lab.test A` | Returned `192.0.2.10` with TTL 60 |
| `ipv6.lab.test AAAA` | Returned `2001:db8::10` with TTL 90 |
| `multi.lab.test A` | Returned three IPv4 answers with TTL 45 |
| `missing.lab.test A` | Returned `STATUS NXDOMAIN` |

## 7. Current Project Status

At the end of this milestone, the project is approximately 50% complete.

Completed so far:

- Project skeleton and Makefile
- `--supported` interface
- Command-line validation
- DNS type conversion
- DNS query construction
- Domain-name encoding
- UDP transmission and reception
- DNS header and question parsing
- Response validation
- Compressed-name decoding
- `A` and `AAAA` answer parsing
- Multiple-answer traversal
- `NXDOMAIN` handling
- Safe unsupported-record skipping

## 8. Remaining Work

The next stage will focus on:

- Parsing `CNAME` records
- Parsing `NS` records
- Parsing the selected optional `MX` record
- Testing records whose RDATA contains compressed names
- Integrating the implementation into the required `src/main.cpp`
- Updating the Makefile to produce the final `dnsclient` executable
- Adding malformed-message and boundary tests
- Completing final Docker and clean-build verification

## 9. Git Milestone

The work for this stage was developed on:

```text
feature/udp-response-parsing
```

Suggested commit message:

```text
Add UDP response parsing and second milestone report
```
