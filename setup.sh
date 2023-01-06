#!/bin/env bash

set -eu

# Install dependencies.
sudo apt install \
    build-essential \
    curl \
    g++-multilib \
    gcc-multilib \
    python3 \
    wget \
    ;

# Extract Intel Pin.
PIN=pin-3.20-98437-gf02b61307-gcc-linux
if [ ! -d "$PIN" ]; then
    tar xvf "$PIN.tar.gz"
fi

# Install & Build libdft64.
# > PIN_ROOT: Specify the location for the Pin kit when building a tool outside of the kit.
# (PIN_ROOT is for libdft64 builds)
# ref. https://software.intel.com/sites/landingpage/pintool/docs/98650/Pin/doc/html/index.html#UsefulVariables
export PIN_ROOT="$PWD/$PIN"
if [ ! -d libdft64 ]; then
    git clone https://github.com/AngoraFuzzer/libdft64
fi
cd libdft64
git checkout 066c9d8b3eaa7eeb353dc51771948f3dc8062f95
make -j"$(nproc)" || true

# Set ptrace_scope as 0 for Intel Pin.
# ref. https://www.kernel.org/doc/Documentation/security/Yama.txt
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null

echo -e "\n\nRun \033[1msource env.init\033[0m\n"

