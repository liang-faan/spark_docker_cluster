# FROM openjdk:8u252-jdk-slim
FROM ubuntu:18.04
#FROM linuxbrew/linuxbrew:latest

RUN apt-get update \
   && yes | apt-get install curl \
   && yes | apt-get install openjdk-8-jdk \
   #&& yes | apt-get install cmake \
   && export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64/ \
   && apt-get -y install git
   #&& yes | apt-get install tar wget git zlib1g-dev autoconf libtool build-essential python-dev python-six python-virtualenv libcurl4-nss-dev libsasl2-dev libsasl2-modules maven libapr1-dev libsvn-dev \
   # && mkdir /spark && mkdir /download \
   # && cd /download \
 #  && brew update
   # && $(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)
   # && curl https://downloads.apache.org/mesos/1.9.0/mesos-1.9.0.tar.gz -O \
   # && tar xvf ./mesos-1.9.0.tar.gz \
   # && cd mesos-1.9.0 \
   # && mkdir build \
   # && cd build

RUN git clone https://github.com/Homebrew/brew /linuxbrew/Homebrew
RUN ls /linuxbrew
# RUN mkdir /.linuxbrew/bin
RUN ls /
RUN ln -s /linuxbrew/Homebrew/bin/brew /bin
RUN eval $(/bin/brew shellenv)
RUN /bin/brew update

#RUN cd /download/mesos-1.9.0/build \
#    && ../configure \
#   && make install
   #&& apt -y install build-essential python-dev libcurl4-nss-dev libsasl2-dev libsasl2-modules maven libapr1-dev libsvn-dev zlib1g-dev


# apt install openjdk-8-jdk -y
# export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64/
# apt -y install build-essential python-dev libcurl4-nss-dev libsasl2-dev libsasl2-modules maven libapr1-dev libsvn-dev zlib1g-dev
# apt install unzip