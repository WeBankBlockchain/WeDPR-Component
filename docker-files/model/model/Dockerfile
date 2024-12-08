FROM wedpr-model-service-base-image:v3.0.0 as wedpr-model-service
LABEL maintainer=service@webank.com

ENV LANG zh_CN.UTF-8
ENV LANGUAGE zh_CN.UTF-8
ENV LC_ALL zh_CN.UTF-8

WORKDIR /data/home/wedpr
ARG SOURCE_BRANCH
ENV DEBIAN_FRONTEND=noninteractive \
    SOURCE=${SOURCE_BRANCH:-main}

# install the gateway sdk
# TODO: replace with pypip
RUN pip install -i https://test.pypi.org/simple/ wedpr-python-gateway-sdk

# obtain the source, no need to fetch the submodule(vcpkg) here
RUN git clone https://github.com/WeBankBlockchain/WeDPR-Component.git -b ${SOURCE}
# move the files to the /data/app path
RUN mkdir -p /data/home/wedpr/wedpr-model/ \
    && mv /data/home/wedpr/WeDPR-Component/python/ppc_common /data/home/wedpr/wedpr-model/ppc_common \
    && mv /data/home/wedpr/WeDPR-Component/python/ppc_model /data/home/wedpr/wedpr-model/ppc_model \
    && mv /data/home/wedpr/WeDPR-Component/python/aes_key.bin /data/home/wedpr/wedpr-model/ppc_model \
    && cp /data/home/wedpr/wedpr-model/ppc_model/tools/*.sh /data/home/wedpr/wedpr-model/ppc_model

# clear the WeDPR-Component
RUN rm -rf /data/home/wedpr/WeDPR-Component

ENTRYPOINT ["/bin/bash", "/data/home/wedpr/wedpr-model/start.sh", "true"]
