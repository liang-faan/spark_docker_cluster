# This file contains environment variables that are passed to mesos-master.
# To get a description of all options run mesos-master --help; any option
# supported as a command-line option is also supported as an environment
# variable.

# Some options you're likely to want to set:
# export MESOS_log_dir=/var/log/mesos

#export MESOS_master=zoomaster:5050
#export MESOS_log_dir=/var/log/mesos/
#export MESOS_work_dir=/var/data/mesos/
#export MESOS_isolation=cgroups

export MESOS_log_dir=/home/$USER/mesos/log
export MESOS_work_dir=/home/$USER/mesos/data
#export MESOS_ZK=zk://zoomaster:2181,zooslave1:2181,zooslave2:2181/mesos
export MESOS_quorum=1
export MESOS_ZK=zk://zoomaster:2181/mesos
