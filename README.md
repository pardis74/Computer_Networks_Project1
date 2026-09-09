# Project 1: DNS Client and Message Parser

**Due: September 30, 2026**

## Overview

In this project, you will implement a small DNS client in **C or C++** from scratch. Your program will construct DNS queries using the standard DNS wire format, send them to a DNS server over UDP, receive the raw response, and parse the response without relying on DNS client/parsing libraries.


Your implementation will be tested automatically against both supplied and hidden DNS messages.


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
- SOA

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

The DNS format details are all in the [DNS RFCs](https://www.rfc-editor.org/rfc/rfc1035).

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

## Recommended Steps

At a high level, work through the project in roughly this order:

1. Accept the assignment and set up your project skeleton (`Makefile`, source files, `--supported` output).
2. Get message parsing working offline first — DNS header, then uncompressed names, then a basic A record.
3. Extend parsing to the rest of the mandatory record types, and safely skipping unsupported records.
4. Add query construction and UDP communication so live queries work end-to-end.
5. Implement your selected optional record type.
6. Test thoroughly against the Docker DNS environment, including malformed and edge-case input.

**Make sure these steps are clearly marked in the commits**, not just as a single final commit. See [Repository Workflow](#repository-workflow-required) below for how that's tracked.

## Repository Workflow (required)

This project is distributed and submitted through **GitHub**, using **[Classroom 50](https://github.com/foundation50/classroom50)** to manage assignment repositories. Your repository *is* your submission, and your commit/issue/PR history is part of how your work is evaluated — not just the final code.

### Setup

1. Accept the course organization invite you're sent. You must join the organization before you can accept any assignment.
2. Use the assignment invite link shared separately to accept this assignment — this creates your own repository under the course organization, seeded from this project.
3. Clone your repository locally and confirm you have push access before you start working.

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


