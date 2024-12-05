FROM wedpr-image:latest as wedpr-model-service

LABEL maintainer service@webank.com

ENV LANG zh_CN.UTF-8
ENV LANGUAGE zh_CN.UTF-8
ENV LC_ALL zh_CN.UTF-8

WORKDIR /data/home/wedpr
ARG SOURCE_BRANCH
ENV DEBIAN_FRONTEND=noninteractive \
    SOURCE=${SOURCE_BRANCH:-master}

RUN mkdir -p /data/home/wedpr
# copy requirements
COPY depends/requirements.txt /data/home/wedpr/requirements.txt

# install the requirements
RUN pip install --no-cache-dir -i https://pypi.mirrors.ustc.edu.cn/simple/ -r /data/home/wedpr/requirements.txt

# obtain the source
RUN git clone https://github.com/WeBankBlockchain/WeDPR-Component.git --recursive --depth=1 -b ${SOURCE}
# move the files to the /data/app path
RUN mkdir -p /data/home/wedpr/wedpr-model/ \
    && mv /data/home/wedpr/WeDPR-Component/python/ppc_common /data/home/wedpr/wedpr-model/ppc-common \
    && mv /data/home/wedpr/WeDPR-Component/python/ppc_model /data/home/wedpr/wedpr-model/ppc-model \
    && mv /data/home/wedpr/WeDPR-Component/python/aes_key.bin /data/home/wedpr/wedpr-model/ppc-model \
    && cp /data/home/wedpr/model/ppc-model/tools/*.sh /data/home/wedpr/wedpr-model/ppc-model

# clear the WeDPR-Component
RUN rm -rf /data/home/wedpr/WeDPR-Component

ENTRYPOINT ["/bin/bash", "/data/home/wedpr/wedpr-model/start.sh", "true"]
