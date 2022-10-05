ARG VARIANT="debian-10"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

RUN export DEBIAN_FRONTEND=noninteractive               && \
    apt-get update                                      && \
    apt-get -y install --no-install-recommends             \
      git wget unzip build-essential lib32z1 libtool       \
      bison bisonc++ libbison-dev autoconf autotools-dev   \
      automake libssl-dev zlib1g-dev flex libfl-dev cmake  \
      yui-compressor closure-compiler optipng jpegoptim    \
      libtidy5deb1 sassc sass-spec libxml2-utils rsync

WORKDIR /opt

RUN wget -q https://github.com/roleoroleo/sonoff-hack/releases/download/toolchain-0.0.1/arm-sonoff-linux-uclibcgnueabi.tgz && \
    tar xf arm-sonoff-linux-uclibcgnueabi.tgz && rm -rf arm-sonoff-linux-uclibcgnueabi.tgz

ENV PATH=/opt/arm-sonoff-linux-uclibcgnueabi/bin:${PATH}

