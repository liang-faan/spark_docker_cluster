# Create docker spark container based on spark version 2.4.6
```
ENV SPARK_HOME=/spark
ENV PATH=${PATH}:${SPARK_HOME}/bin:${SPARK_HOME}/sbin

RUN curl https://apachemirror.sg.wuchna.com/spark/spark-2.4.6/spark-2.4.6-bin-hadoop2.7.tgz -O \
    && tar -C /spark -xvf spark-2.4.6-bin-hadoop2.7.tgz \
    && rm spark-2.4.6-bin-hadoop2.7.tgz \
    && mv /spark/spark-2.4.6-bin-hadoop2.7/* /spark


```

# Create docker-compose to have spark master - slave clusters.  
## Based on node type to start different spark servers. 
```
if [ "$SPARKNODE" = "MASTER" ]; then
  echo "starting spark master"
    /spark/sbin/start-master.sh 
    tail -f /spark/logs/spark--org.apache.spark.deploy.master.Master-1-$HOST_NAME.out
fi


if [ "$SPARKNODE" = "SLAVE" ]; then
  echo "starting spark slave"
    /spark/sbin/start-slave.sh spark://$MASTER_HOST_NAME:7077 --webui-port $WEBUIPORT
    tail -f /spark/logs/spark--org.apache.spark.deploy.worker.Worker-1-$HOST_NAME.out
fi
```

## start up docker compose
```
cd ./compose
docker-compose up
```
