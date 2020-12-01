FROM ubuntu:18.04
# Install Prerequisites
  
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y git  openssl cmake wget \
    libssl-dev build-essential  && \
    apt-get clean -y
# Install Dependencies <TO DO>
WORKDIR /home
