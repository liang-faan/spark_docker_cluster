# FROM ubuntu:18.04

# RUN apt-get update && \
#     apt-get install build-essential curl file git ruby-full locales --no-install-recommends -y && \
#     rm -rf /var/lib/apt/lists/*

# RUN localedef -i en_US -f UTF-8 en_US.UTF-8

# RUN useradd -m -s /bin/bash linuxbrew && \
#     echo 'linuxbrew ALL=(ALL) NOPASSWD:ALL' >>/etc/sudoers

# USER linuxbrew
# RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/install/master/install.sh)"

# USER root
# ENV PATH="/home/linuxbrew/.linuxbrew/bin:${PATH}"

# RUN apt-get update \
#     && yes | apt-get install openjdk-8-jdk \
#     && export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

# RUN brew install mesos

FROM centos:latest

RUN yum -y update

RUN yum install -y java-1.8.0-openjdk

COPY ./mesos-1.9.0-1.el7.x86_64.rpm /tmp

#RUN curl -O https://apache.bintray.com/mesos/el7/x86_64/mesos-1.9.0-1.el7.x86_64.rpm

#RUN yum -y update \
# && yum -y install mesos


RUN rpm –i /tmp/mesos-1.9.0-1.el7.x86_64.rpm
