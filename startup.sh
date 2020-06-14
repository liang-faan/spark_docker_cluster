#!/bin/bash

echo "come into startup script ...."

chmod +x /spark/bin/*.sh

chmod +x /spark/sbin/*.sh

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