FROM ubuntu:latest

USER root

WORKDIR opt

ENV TZ=Europe/Prague

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    apt update && apt install yui-compressor libxml2-utils git wget lib32z1 -y && apt clean

RUN wget -q https://github.com/roleoroleo/sonoff-hack/releases/download/toolchain-0.0.1/arm-sonoff-linux-uclibcgnueabi.tgz && \
    tar xf arm-sonoff-linux-uclibcgnueabi.tgz && rm -rf arm-sonoff-linux-uclibcgnueabi.tgz 

ENV PATH=${PATH}:/opt/arm-sonoff-linux-uclibcgnueabi/bin/
