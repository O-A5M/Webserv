FROM ubuntu:22.04

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        build-essential \
        lldb \
        valgrind \
        git \
        cmake \
        curl \
    && apt-get clean && rm -rf /var/lib/apt/lists/*