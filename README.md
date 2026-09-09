# Project 1: DNS Client and Message Parser

**Due: September 30, 2026**

## Overview

In this project, you will implement a small DNS client in **C or C++** from scratch. Your program will construct DNS queries using the standard DNS wire format, send them to a DNS server over UDP, receive the raw response, and parse the response without relying on DNS client/parsing libraries.

The goal is not to implement a complete DNS resolver. Instead, you will implement a carefully selected subset of DNS that exercises application-layer protocol design, binary message formats, network byte order, variable-length fields, UDP communication, and DNS name compression.

Your implementation will be tested automatically against both supplied and hidden DNS messages.

## Learning Objectives

By completing this project, you should be able to:

- Explain how an application-layer protocol is represented on the wire.
- Construct and parse binary protocol messages.
- Work correctly with network byte order.
- Encode and decode DNS domain names.
- Interpret DNS headers, questions, and resource records.
- Decode DNS name-compression pointers.
- Send and receive DNS messages over UDP.
- Safely skip DNS record types that your program does not interpret.

## Rules

Your submitted program must construct and interpret DNS messages itself.

**External Networking Tools** You may use Wireshark, tcpdump, `dig`, `host`, `nslookup`, or similar tools to observe DNS traffic and prepare/inspect test data. You may not use them to produce the required assignment output — your program itself must construct the query and parse the response.

You may not use libraries that construct or parse DNS messages for you. Standard socket APIs, standard networking headers, and standard address-formatting routines such as `inet_ntop()` are allowed.

## Required DNS Record Types

Every implementation must support:

1. **A** - IPv4 address
2. **AAAA** - IPv6 address
3. **CNAME** - canonical/alias name
4. **NS** - authoritative name-server name

In addition, you must implement **one** of the following record types:

- MX
- TXT
- PTR
- SRV
- SOA
- CAA

Your program must report its selected type through the `--supported` command described below.

You do **not** need to interpret every possible DNS record type. However, your parser must be able to safely skip an unsupported resource record and continue parsing subsequent records by using the record's `RDLENGTH` field.

## Required Program Interface

Your executable must be named `dnsclient`.

### Query mode

```bash
./dnsclient --query <dns-server> <hostname> <type>
```

Example:

```bash
./dnsclient --query 10.0.0.53 www.example.test A
```

The program must:

1. construct the DNS query;
2. send it over UDP to port 53;
3. receive the response;
4. verify the response;
5. parse the DNS message; and
6. print the result using the required output format.

### Parse-file mode

```bash
./dnsclient --parse <packet-file>
```

The file contains exactly one raw DNS message, beginning with the DNS header. There is no Ethernet, IP, or UDP header in this file.

This mode allows the DNS parser to be tested independently of socket communication.

### Supported-types mode

```bash
./dnsclient --supported
```

Example output for a group choosing MX:

```text
A AAAA CNAME NS MX
```

Print the four mandatory types first, followed by exactly one selected type.

## Required Output

Output must be machine-readable. Do not add decorative text to standard output.

Example:

```text
STATUS NOERROR
QUESTION www.example.test A
ANSWER www.example.test CNAME TTL=120 VALUE=web.example.test
ANSWER web.example.test A TTL=60 VALUE=10.0.0.17
```

Multiple answers must appear on separate `ANSWER` lines.

NXDOMAIN example:

```text
STATUS NXDOMAIN
QUESTION missing.example.test A
```

AAAA example:

```text
STATUS NOERROR
QUESTION ipv6.example.test AAAA
ANSWER ipv6.example.test AAAA TTL=300 VALUE=2001:db8::10
```

For the selected optional type, additional fields should use `KEY=VALUE` form. The exact expected formats will be supplied with the test data for each optional type.

Diagnostic/debug output should be written to `stderr`, not `stdout`.

## DNS Functionality You Must Implement

Think of this as building the parsing/construction logic behind a tool like `dig` — the same information a `dig` response shows you (header flags and counts, the question, and the answer records with their names, types, TTLs, and values, including compressed names) — just emitted in this project's own output format instead of `dig`'s. That means correctly handling the DNS header, domain-name encoding/decoding (including compression pointers), and resource records for all required types, while safely skipping any record type you don't support.

The wire format details (header layout, label encoding, record structure, compression pointer bits, etc.) are all in the [DNS RFCs](https://www.rfc-editor.org/rfc/rfc1035) and widely available references — figuring those out is part of the assignment.

## Out of Scope

You are **not** required to implement:

- recursive or iterative resolution yourself;
- TCP-based DNS;
- DNSSEC;
- EDNS/OPT interpretation;
- DNS-over-HTTPS or DNS-over-TLS;
- a caching resolver;
- arbitrary DNS record types;
- production-grade protection against every malicious DNS packet.

Your code should nevertheless perform reasonable length/bounds checks and must not intentionally read outside the received message.

## Step-by-Step Instructions

Work through these steps in order. Each step should be its own GitHub issue and its own branch/PR — see [Repository Workflow](#repository-workflow-required) below.

1. **Get access to your repository and clone it.** Follow your instructor's instructions for obtaining your project repository, then clone it locally.
2. **Set up your project skeleton.** Create `src/main.c`, `src/dns.c`, and `src/dns.h` (or the C++ equivalents), plus a `Makefile` that builds an executable named `dnsclient`. Commit this as your first PR.
3. **Parse a supplied simple A-response file.** Write a raw DNS response to a file and get `--parse` reading bytes from it before you write any protocol logic.
4. **Implement DNS header parsing.** Read and validate the 12-byte header (ID, flags, counts).
5. **Implement uncompressed domain-name decoding.** Handle the length-prefixed label format ending in a zero byte.
6. **Implement A record parsing.** Parse the question section and a single A answer; produce the required `STATUS`/`QUESTION`/`ANSWER` output.
7. **Construct an A query.** Build the header, question, and hostname encoding for an outgoing query.
8. **Add UDP communication.** Send the query to the target server on port 53 and receive the raw response.
9. **Implement DNS name compression.** Handle pointers, and names made of labels followed by a pointer.
10. **Add AAAA, CNAME, and NS parsing.** Extend your resource-record logic to cover all four mandatory types.
11. **Correctly skip unsupported records.** Use `RDLENGTH` to advance past any record type you don't interpret, without stopping parsing.
12. **Implement your group's selected optional type** (MX, TXT, PTR, SRV, SOA, or CAA) and report it via `--supported`.
13. **Handle transaction-ID verification and NXDOMAIN.** Reject mismatched responses; print the correct output for NXDOMAIN.
14. **Test against the Docker DNS environment** (see below), including malformed/truncated input and error paths.
15. **Review your issue/PR history** to make sure your work is fully logged before the deadline (see below).

## Repository Workflow (required)

This project is distributed and submitted through **GitHub**. Your repository *is* your submission, and your commit/issue/PR history is part of how your work is evaluated — not just the final code.

### Setup

1. Get access to your repository following your instructor's instructions and clone it.
2. Confirm you have push access before you start working.

### Track your work with Issues

- Before starting a step from the list above (or any nontrivial piece of work), **open a GitHub Issue** describing what you're about to do (e.g., "Implement uncompressed name decoding").
- Use issues to track bugs you find, design decisions, and open questions — not just planned features.
- Close each issue by referencing it from the commit or PR that resolves it (e.g., `Closes #4`).

### Do work on branches and Pull Requests

- Do not commit directly to `main`. Create a feature branch per issue (e.g., `feature/header-parsing`, `feature/name-compression`).
- Open a Pull Request from your branch back into `main` for every meaningful chunk of work.
- Write a short PR description explaining what changed and why. Merge the PR once it builds and works as expected.
- If working in a group, have a teammate review the PR before merging when possible.

### Log your progress

- Keep commit messages specific and incremental (avoid single giant commits with the whole project in them).
- The combination of your issues, branches, PRs, and commits should tell the story of how the implementation evolved — this history may be reviewed as part of grading, alongside the final code.

## Build

Your program must build with:

```bash
make
```

and produce:

```text
./dnsclient
```

Your submission must build non-interactively using `make` inside the student container (see [DOCKER.md](DOCKER.md)) — grading uses this same container.

## Submission

Your GitHub repository **is** your submission. Make sure your final code is pushed to `main` before the deadline, and that it builds with `make` from a clean clone. Do not commit compiled binaries, packet captures, generated files, or third-party DNS libraries.

## Docker DNS Test Environment

A Docker-based DNS server is included so that everyone can test against the same deterministic DNS data over real UDP DNS traffic, using the standard DNS wire format.

See [DOCKER.md](DOCKER.md) for how to start the environment, the supplied test records, and Docker installation instructions.


