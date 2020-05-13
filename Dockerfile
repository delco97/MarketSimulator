# Start from the ubuntu official image
FROM ubuntu:16.04
# Setup image
RUN apt-get update && apt-get install -y \
    gcc \
    valgrind \
    make
#Define starting working directory inside image
WORKDIR /usr/src