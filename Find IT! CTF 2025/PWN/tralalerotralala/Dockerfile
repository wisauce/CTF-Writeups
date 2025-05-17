FROM ubuntu:22.04@sha256:b6b83d3c331794420340093eb706a6f152d9c1fa51b262d9bf34594887c2c7ac

RUN apt-get update && apt-get install -y socat

RUN adduser -u 1337 tralalerotralala

WORKDIR /home/tralalerotralala

COPY ./flag ./flag
COPY ./deploy/chall ./chall

RUN chmod 644 ./flag 
RUN chmod 755 ./chall

USER tralalerotralala

EXPOSE 8080
CMD socat TCP-LISTEN:8080,reuseaddr,fork EXEC:./chall,stderr
