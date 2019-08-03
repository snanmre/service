# service
Simpler version of upstart

## Description

With this program, you can start other programs/scripts as a daemon,
save logs and respawn if ended unexpectedly. In general, this runs
as a simpler version of upstart.

## Details

### Related directories/files
* DIRPATH_SERVICES("./services"): directory path of service configuration files
* DIRPATH_SERVICE_SCRIPTS("/run/shm/service"): directory of service script files
* DIRPATH_SERVICE_PIDS("/run/service"): directory of service pid files
* FILEPATH_SERVICES_LIST("/run/service/services.list"): file that contains running services

### Defining a service
Service configuration files must have ".conf" extension.<br/>
Sample files:
* [sample1.conf](https://github.com/snanmre/service/blob/master/samples/services/sample1.conf)
* [sample2.conf](https://github.com/snanmre/service/blob/master/samples/services/sample2.conf)
#### Sample configuration file
```
# execute command
exec sleep 5

# or, you can define a script
#script
#  echo "starting..."
#  sleep 5
#end script


# log file path(default /dev/null)
# log /run/sample1.log


# wipe log on start/restart
# wipe log


# pid file path (default: /run/service/<service_name>.pid)
# pidfile /run/sample1.pid


# respawn service if it dies
respawn


# set respawn options
#    limit: respawn limit count, default: int max
#    interval: delay before start in seconds: 0
#
#    respawn limit <limit> <interval
respawn limit 5 1
```
