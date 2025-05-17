FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update --fix-missing
RUN apt-get -y upgrade
RUN apt-get -y install xinetd
RUN apt-get install -y qemu-system seabios libcurl4-openssl-dev libfuse3-dev libaio-dev liburing-dev libpipewire-0.3-dev libjack-dev libpulse-dev libasound-dev libdw-dev python3 libvdeplug-dev libslirp-dev liblzo2-2 libdaxctl-dev libnuma-dev libkeyutils-dev libusb-1.0-0-dev libsnappy-dev libpixman-1-dev libcapstone-dev libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev ninja-build libsasl2-dev libbpf-dev && rm -rf /var/lib/apt/lists/*
RUN groupadd -r pwn && useradd -r -g pwn pwn

ADD --chmod=640 build/pwn.xinetd /etc/xinetd.d/pwn
ADD --chmod=700 build/init.sh /etc/init.sh
RUN chmod 1733 /tmp /var/tmp /dev/shm

WORKDIR /home/pwn
ADD release .
RUN chmod a+x qemu-system-x86_64 run.sh

ARG FLAG
RUN echo "${FLAG}" > flag.txt
RUN chmod 440 flag.txt
RUN mv flag.txt flag-$(md5sum flag.txt | awk '{print $1}').txt

RUN chown -R root:pwn /home/pwn
RUN chmod -R go-w /home/pwn
