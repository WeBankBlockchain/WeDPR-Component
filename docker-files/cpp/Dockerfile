FROM ubuntu:18.04 as builder

LABEL maintainer service@webank.com

WORKDIR /

ARG SOURCE_BRANCH
ENV DEBIAN_FRONTEND=noninteractive \
    SOURCE=${SOURCE_BRANCH:-master}

RUN apt-get -q update && apt-get install -qy --no-install-recommends \
    vim curl git make build-essential cmake  \
    libgmp-dev flex bison patch libzstd-dev unzip ninja-build pkg-config curl zip tar ccache uuid-runtime automake autoconf \
    m4 tcpdump net-tools libkrb5-dev krb5-user pkg-config default-libmysqlclient-dev gcc g++ \
    && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
    && apt-get install -qy --no-install-recommends tzdata \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    && rm -rf /var/lib/apt/lists/*

# install rust
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y

ENV PATH="/root/.cargo/bin:${PATH}"

ENV VCPKG_FORCE_SYSTEM_BINARIES=1

RUN git clone https://github.com/WeBankBlockchain/WeDPR-Component.git --recursive --depth=1 -b ${SOURCE} \
    && mkdir -p WeDPR-Component/cpp/build && cd WeDPR-Component/cpp/build \
    && cmake .. -DBUILD_STATIC=ON && make -j8 && cat /WeDPR-Component/cpp/build/*.log


FROM ubuntu:18.04
LABEL maintainer service@webank.com
    
RUN apt-get -q update && apt-get install -qy --no-install-recommends vim libkrb5-dev krb5-user pkg-config default-libmysqlclient-dev \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*
    
COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-gateway-service /usr/local/bin/ppc-gateway-service
COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-pro-node /usr/local/bin/ppc-pro-node
COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-air-node /usr/local/bin/ppc-air-node
COPY --from=builder /WeDPR-Component/cpp/build/bin/wedpr-mpc /usr/local/bin/wedpr-mpc