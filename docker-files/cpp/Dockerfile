FROM fiscoorg/wedpr-component-vcpkg-cache:v3.0.0 as builder

LABEL maintainer=service@webank.com

WORKDIR /

ARG SOURCE_BRANCH
ENV DEBIAN_FRONTEND=noninteractive \
    SOURCE=${SOURCE_BRANCH:-main}

RUN git clone https://github.com/WeBankBlockchain/WeDPR-Component --recursive --depth=1 -b ${SOURCE}  \
# ln the cached vcpkg and compile
RUN cd WeDPR-Component/cpp && rm -rf vcpkg && ln -s /vcpkg  \
    && mkdir -p build && cd build  \
    && cmake .. -DBUILD_STATIC=ON  \
    && make -j8

FROM ubuntu:22.04 as wedpr-gateway-service
LABEL maintainer=service@webank.com
    
RUN apt-get -q update && apt-get install -qy --no-install-recommends vim \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /data/home/wedpr/wedpr-gateway-service

COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-gateway-service /data/home/wedpr/wedpr-gateway-service/ppc-gateway-service

ENTRYPOINT ["/data/home/wedpr/wedpr-gateway-service/ppc-gateway-service", "-c", "config.ini"]

FROM ubuntu:22.04 as wedpr-pro-node-service
LABEL maintainer=service@webank.com

RUN apt-get -q update && apt-get install -qy --no-install-recommends vim libkrb5-dev krb5-user \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /data/home/wedpr/wedpr-pro-node-service

COPY --from=builder /WeDPR-Component/cpp/build/bin/ppc-pro-node /data/home/wedpr/wedpr-pro-node-service/ppc-pro-node

ENTRYPOINT ["/data/home/wedpr/wedpr-pro-node-service/ppc-pro-node", "-c", "config.ini"]

FROM ubuntu:22.04 as wedpr-mpc-service
LABEL maintainer=service@webank.com

RUN apt-get -q update && apt-get install -qy --no-install-recommends vim libkrb5-dev krb5-user \
        && ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime \
        && apt-get install -qy --no-install-recommends tzdata \
        && dpkg-reconfigure --frontend noninteractive tzdata \
        && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /data/home/wedpr/wedpr-mpc-service

COPY --from=builder /WeDPR-Component/cpp/build/bin/wedpr-mpc /data/home/wedpr/wedpr-mpc-service/wedpr-mpc

ENTRYPOINT ["/data/home/wedpr/wedpr-mpc-service/wedpr-mpc", "-c", "config.ini"]
