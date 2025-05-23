FROM ubuntu:22.04 as builder

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

RUN git clone https://github.com/WeBankBlockchain/WeDPR-Component --recursive --depth=1 -b ${SOURCE} 

# ln the cached vcpkg and compile
RUN cd WeDPR-Component/cpp  \
    && mkdir -p build && cd build  \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON  \
    && make -j8

# remove the vcpkg
RUN rm -rf /WeDPR-Component/cpp/vcpkg

FROM ubuntu:22.04 as wedpr-gateway-service
LABEL maintainer=service@webank.com
    
RUN apt-get -q update && apt-get install -qy --no-install-recommends vim \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-gateway-service /usr/local/bin/ppc-gateway-service

ENTRYPOINT ["/usr/local/bin/ppc-gateway-service"]
CMD ["--version"]

FROM ubuntu:22.04 as wedpr-pro-node-service
LABEL maintainer=service@webank.com

RUN apt-get -q update && apt-get install -qy --no-install-recommends vim libkrb5-dev \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-pro-node /usr/local/bin/ppc-pro-node

ENTRYPOINT ["/usr/local/bin/ppc-pro-node"]
CMD ["--version"]

FROM fiscoorg/wedpr-mpc-base-ubuntu:v3.0.0 as wedpr-mpc-service
LABEL maintainer=service@webank.com

RUN apt-get -q update && apt-get install -qy --no-install-recommends vim libkrb5-dev python3.11 python3-pip python3-dev telnet tcpdump net-tools\
        && rm -f /usr/bin/python && ln -s /usr/bin/python3.11 /usr/bin/python  \
        && rm -f /usr/bin/python3 && ln -s /usr/bin/python3.11 /usr/bin/python3  \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

COPY --from=builder /WeDPR-Component/cpp/build/bin/wedpr-mpc /usr/local/bin/wedpr-mpc

ENTRYPOINT ["/usr/local/bin/wedpr-mpc"]
CMD ["--version"]
