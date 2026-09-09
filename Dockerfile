FROM gcc:14-bookworm

WORKDIR /project

RUN apt-get update \
    && apt-get install -y --no-install-recommends make tcpdump iproute2 \
    && rm -rf /var/lib/apt/lists/*

COPY . /project
RUN make clean && make

CMD ["/bin/bash"]
