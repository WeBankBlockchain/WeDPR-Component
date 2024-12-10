FROM ubuntu:22.04 as wedpr-component-vcpkg-cache

LABEL maintainer=service@webank.com

WORKDIR /

ARG SOURCE_BRANCH
ENV DEBIAN_FRONTEND=noninteractive \
    SOURCE=${SOURCE_BRANCH:-main}
# Note: depends on python3; default jdk for jni
RUN apt-get -q update && apt-get install -qy --no-install-recommends \
    vim curl lcov git make nasm build-essential cmake wget libtool ca-certificates python3.11 python3-dev \
    libgmp-dev flex bison patch libzstd-dev unzip ninja-build pkg-config zip tar ccache uuid-runtime automake autoconf \
    m4 tcpdump net-tools gcc g++ default-jdk \
    && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
    && apt-get install -qy --no-install-recommends tzdata \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    && rm -rf /var/lib/apt/lists/*

# install rust
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y

ENV PATH="/root/.cargo/bin:${PATH}"

ENV VCPKG_FORCE_SYSTEM_BINARIES=1

# first download and compile the vcpkg dependecies
RUN git clone https://github.com/cyjseagull/WeDPR-Component --recursive --depth=1 -b ${SOURCE} \
    && mkdir -p WeDPR-Component/cpp/build && cd WeDPR-Component/cpp/build \
    && cmake .. -DBUILD_STATIC=ON

# move the vcpkg dependecies to /vcpkg
RUN mv /WeDPR-Component/cpp/vcpkg /

# remove the WeDPR-Component
RUN rm -rf /WeDPR-Component/ && rm -rf /vcpkg/buildtrees/*/src && rm -rf /vcpkg/buildtrees/*/*-dbg

