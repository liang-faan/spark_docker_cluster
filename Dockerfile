FROM openjdk:8u252-jdk-slim

#FROM ubuntu:18.04

#ARG spark_uid=185

# install packages
#RUN apt-get update && \
   # apt-get install -y curl \
  #  wget \
  #  openjdk-8-jdk

#RUN apt-get update && apt-get install -y openjdk-8-jdk \
#   && yes | apt-get install curl 

RUN apt-get update \
    && yes | apt-get install -y procps \
    && yes | apt-get install curl \
    && mkdir /spark && mkdir /download

#ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
#ENV PATH=${JAVA_HOME}/bin:${PATH}

WORKDIR /

#RUN mkdir /spark && mkdir /download

ENV SPARK_HOME=/spark
ENV PATH=${PATH}:${SPARK_HOME}/bin:${SPARK_HOME}/sbin

WORKDIR /download

RUN curl https://apachemirror.sg.wuchna.com/spark/spark-2.4.6/spark-2.4.6-bin-hadoop2.7.tgz -O \
    && tar -C /spark -xvf spark-2.4.6-bin-hadoop2.7.tgz \
    && rm spark-2.4.6-bin-hadoop2.7.tgz \
    && mv /spark/spark-2.4.6-bin-hadoop2.7/* /spark

#RUN start-master.sh

WORKDIR ../spark

COPY startup.sh /spark

RUN chmod +x /spark/startup.sh

ENTRYPOINT ["/spark/startup.sh"]
