# Modify Hostname
```

## change the username to zoomaster
# sudo nano /etc/hostname

## Update hosts 
# *** sudo nano /etc/hosts *** # no need set the hostname map to 127.0.0.1, otherwise will have Zookeeper error: Cannot open channel to X at election address


# Master = zoomaster
# Slave = zooslave1, zooslave2, zooslave3, ...

```

# Zookeeper configuration

```
## update Zookeeper configuration file
# sudo nano /etc/zookeeper/conf/zoo.cfg
# server.1=zoomaster:2888:3888
# server.2=zooslave1:2888:3888
# server.3=zooslave2:2888:3888

## zoomaster
# echo 1 | sudo tee /etc/zookeeper/conf/myid

## zooslave1
# echo 2 | sudo tee /etc/zookeeper/conf/myid

## zooslave2
# echo 3 | sudo tee /etc/zookeeper/conf/myid

```

# Restart zookeeper service

```
## List all service 
# service --status-all
# service zookeeper restart
```

# Monitory zookeeper logs
```
tail -f /var/log/zookeeper/zookeeper.log
```

# Mesos configuration

```
cd /usr/etc/mesos/

sudo cp mesos-master-env.sh.template mesos-master-env.sh
sudo cp mesos-deploy-env.sh.template mesos-deploy-env.sh
sudo cp mesos-agent-env.sh.template mesos-agent-env.sh
sudo cp mesos-slave-env.sh.template mesos-slave-env.sh

echo zoomaster | sudo tee masters
echo zooslave1 | sudo tee slaves
echo zooslave2 | sudo tee slaves

```

# update mesos master configuration mesos-master-env.sh

```
# mkdir -p /var/data/mesos
# chmod 777 /var/data/mesos

export MESOS_log_dir=/var/log/mesos/
export MESOS_work_dir=/var/data/mesos/
export MESOS_ZK=zk://zoomaster:2181,zooslave1:2181,zooslave2:2181/mesos
export MESOS_quorum=1
```


# update mesos slave configuration mesos-agent-env.sh, mesos-slave-env.sh
``` 

# to allow mesos agent start without root permission, mesos log & data folder must be have write permission
# below /var/log/mesos will cause mesos agent start failed because these folder requires root permission

export MESOS_master=zoomaster:5050
#export MESOS_log_dir=/var/log/mesos/
#export MESOS_work_dir=/var/data/mesos/
export MESOS_log_dir=/home/$USER/mesos/log
export MESOS_work_dir=/home/$USER/mesos/data
#isolation requires root permistion to execute 
#export MESOS_isolation=cgroups/cpu
export MESOS_systemd_enable_support=false
```

# Start Mesos
```
#login to zoomaster
mesos start-cluster.sh
```

# stop mesos
```
# login zoomaster
mesos stop-cluster.sh
```


# check mesos process
```
ps -ef | grep mesos
```

# Issues & Solution

```
## There is require ssh password when start salves 
Starting mesos-agent on zooslave1
ssh -o StrictHostKeyChecking=no -o ConnectTimeout=2 zooslave1 /usr/sbin/mesos-daemon.sh mesos-agent </dev/null >/dev/null
lanphan@zooslave1's password: Starting mesos-agent on zooslave2
ssh -o StrictHostKeyChecking=no -o ConnectTimeout=2 zooslave2 /usr/sbin/mesos-daemon.sh mesos-agent </dev/null >/dev/null

lanphan@zooslave2's password: Permission denied, please try again.

lanphan@zooslave1's password: Permission denied, please try again.

# This is due to SSH requires the password

## solution
#login zoomaster

ssh-keygen
ssh-copy-id zooslave1
ssh-copy-id zooslave2

```

