# Docker DNS Test Environment

A Docker-based DNS server is included so that everyone can test against the same deterministic DNS data. The server uses the **standard DNS wire format**; there is no custom protocol.

The supplied BIND server is authoritative for:

```text
lab.test
```

Inside the Docker network, the DNS server has the fixed address:

```text
10.53.0.53
```

## Start the environment

```bash
docker compose up -d dns
```

Check that the DNS container is healthy:

```bash
docker compose ps
```

## Build and run your program in the student container

Open a shell:

```bash
docker compose run --rm student
```

Then build normally:

```bash
make
```

Once your implementation is complete, example queries include:

```bash
./dnsclient --query 10.53.0.53 simple.lab.test A
./dnsclient --query 10.53.0.53 ipv6.lab.test AAAA
./dnsclient --query 10.53.0.53 alias.lab.test CNAME
./dnsclient --query 10.53.0.53 lab.test NS
./dnsclient --query 10.53.0.53 multi.lab.test A
```

Optional-type examples are:

```bash
./dnsclient --query 10.53.0.53 mail.lab.test MX
./dnsclient --query 10.53.0.53 txt-demo.lab.test TXT
./dnsclient --query 10.53.0.53 ptr-demo.lab.test PTR
./dnsclient --query 10.53.0.53 _service._tcp.lab.test SRV
./dnsclient --query 10.53.0.53 lab.test SOA
./dnsclient --query 10.53.0.53 caa-demo.lab.test CAA
```

`missing.lab.test` is intentionally undefined and can be used to test `NXDOMAIN` handling.

## Supplied DNS records

The public Docker zone deliberately contains examples of the required record structures:

| Name | Type | Purpose |
|---|---|---|
| `simple.lab.test` | A | Basic IPv4 response |
| `ipv6.lab.test` | AAAA | IPv6 response |
| `alias.lab.test` | CNAME | Name-based RDATA/compression |
| `lab.test` | NS | Multiple NS records |
| `multi.lab.test` | A | Multiple-answer parsing |
| `mail.lab.test` | MX | Optional MX format |
| `txt-demo.lab.test` | TXT | Optional TXT format |
| `ptr-demo.lab.test` | PTR | Optional PTR format |
| `_service._tcp.lab.test` | SRV | Optional SRV format |
| `lab.test` | SOA | Optional SOA format |
| `caa-demo.lab.test` | CAA | Optional CAA format |
| `missing.lab.test` | - | NXDOMAIN |

These records are **examples, not the autograder's complete test set**. Hidden grading will use different names, values, TTLs, record ordering, and compression layouts. Do not hard-code any supplied record data.

## Why the resolver is Dockerized

The Docker DNS server gives each group the same reproducible environment while still exercising real UDP DNS traffic:

```text
student dnsclient
       |
       | UDP/53, standard DNS
       v
BIND authoritative server
       |
       +-- lab.test zone
```

The public server exists for development only. The autograder may launch a separate DNS server with hidden zone data and run the same required command-line interface against it.

## DNS debugging tools

`tcpdump` is installed in the student image for packet-level debugging. Wireshark, `dig`, `host`, and `nslookup` are not installed in the student container, but you're free to use them elsewhere (e.g., your host machine) to observe DNS traffic and prepare or inspect test data. They may not be used to produce the required assignment output — your `dnsclient` program must construct the query and parse the response itself.

## Stop the environment

```bash
docker compose down
```

## Installing Docker

You need Docker Desktop (or the Docker Engine + Compose plugin on Linux) to use this environment. `docker compose` (with a space) is the version used throughout this guide — make sure your install provides it rather than only the older standalone `docker-compose`.

### macOS

1. Install [Docker Desktop for Mac](https://www.docker.com/products/docker-desktop/) (choose the Apple Silicon or Intel build to match your machine).
2. Launch Docker Desktop once and wait for it to report "Docker is running."
3. Verify from a terminal:

   ```bash
   docker --version
   docker compose version
   ```

### Windows

1. Install [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop/). WSL 2 is required — the installer will prompt you to enable it if it isn't already.
2. Launch Docker Desktop and wait for it to report "Docker is running."
3. Run the commands in this guide from a WSL 2 terminal (e.g., Ubuntu) or PowerShell.
4. Verify:

   ```bash
   docker --version
   docker compose version
   ```

### Linux

1. Install Docker Engine for your distribution by following the [official instructions](https://docs.docker.com/engine/install/) (Ubuntu, Debian, Fedora, etc. each have their own page).
2. Install the Compose plugin (usually `docker-compose-plugin` from your package manager, or bundled with the Docker Engine install above).
3. Add your user to the `docker` group so you don't need `sudo` for every command, then log out and back in:

   ```bash
   sudo usermod -aG docker $USER
   ```

4. Verify:

   ```bash
   docker --version
   docker compose version
   ```

### Sanity check

From the project root, confirm the environment starts correctly before you begin development:

```bash
docker compose up -d dns
docker compose ps
docker compose down
```

If `docker compose ps` shows the `dns` service as healthy, your Docker setup is ready.
