FROM ubuntu:noble-20240801

RUN apt-get update && \
    apt-get install -y socat && \
    apt-get clean

RUN echo "Hello" > /home/ubuntu/test

ENTRYPOINT socat TCP-LISTEN:4000,reuseaddr,fork EXEC:"/bin/bash -i",pty,stderr,setsid,sigint,sane,echo=0
