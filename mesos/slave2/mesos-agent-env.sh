# This file contains environment variables that are passed to mesos-agent.
# To get a description of all options run mesos-agent --help; any option
# supported as a command-line option is also supported as an environment
# variable.

# You must at least set MESOS_master.

# The mesos master URL to contact. Should be host:port for
# non-ZooKeeper based masters, otherwise a zk:// or file:// URL.
#export MESOS_master=unknown-machine:5050

# Other options you're likely to want to set:
# export MESOS_log_dir=/var/log/mesos
# export MESOS_work_dir=/var/run/mesos
# export MESOS_isolation=cgroups
#export MESOS_master=zoomaster:5050
#export MESOS_log_dir=/var/log/mesos/
#export MESOS_work_dir=/var/data/mesos/
#export MESOS_isolation=cgroups/cpu
#export MESOS_systemd_enable_support=false


export MESOS_master=zoomaster:5050
#export MESOS_log_dir=/var/log/mesos/
#export MESOS_work_dir=/var/data/mesos/
export MESOS_log_dir=/home/$USER/mesos/log
export MESOS_work_dir=/home/$USER/mesos/data
#isolation requires root permistion to execute 
#export MESOS_isolation=cgroups/cpu
export MESOS_systemd_enable_support=false
