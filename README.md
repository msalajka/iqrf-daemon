# IQRF GW daemon

[![Build Status](https://travis-ci.org/iqrfsdk/iqrf-daemon.svg?branch=master)](https://travis-ci.org/iqrfsdk/iqrf-daemon)

IQRF GW daemon with the multiple communication channels - UDP/MQ/MQTT.

<a href="https://github.com/iqrfsdk/iqrf-daemon/blob/master/doc/iqrf-linux-gw.png" target="_blank">See the daemon architecture</a><br/>
<a href="https://github.com/iqrfsdk/iqrf-daemon/blob/master/doc/iqrf-daemon-component-model.png" target="_blank">See the daemon component model</a><br/>
<a href="https://github.com/iqrfsdk/iqrf-daemon/blob/master/doc/iqrf-daemon-component-instances.png" target="_blank">See the daemon component instances</a><br/>

## How to install the daemon

[http://repos.iqrfsdk.org/](http://repos.iqrfsdk.org/)

-   iqrf-daemon_0.1.2-1_amd64.deb
-   iqrf-daemon_0.1.2-3_armhf.deb
-   libpaho.mqtt.c_1.1.0_amd64.deb
-   libpaho.mqtt.c_1.1.0-1_armhf.deb

### Download public key to verify the packages from the repository

```Bash
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 66CB9A85
```

### Add repository to the source list

-	For Debian (amd64)

```Bash
echo "deb http://repos.iqrfsdk.org/debian jessie testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Ubuntu (amd64)

```Bash
echo "deb http://repos.iqrfsdk.org/ubuntu xenial testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Raspbian (armhf)

```Bash
echo "deb http://repos.iqrfsdk.org/raspbian jessie testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

### Install or update the daemon

```Bash
sudo apt-get install iqrf-daemon
```
or
```Bash
sudo apt-get update
sudo apt-get upgrade iqrf-daemon
```

### Check the daemon service status

```Bash
sudo systemctl status iqrf-daemon.service

● iqrf-daemon.service - IQRF daemon iqrf_startup
   Loaded: loaded (/lib/systemd/system/iqrf-daemon.service; enabled; vendor preset: enabled)
   Active: active (running) since Čt 2017-03-30 23:46:50 CEST; 2min 58s ago
 Main PID: 9540 (iqrf_startup)
   CGroup: /system.slice/iqrf-daemon.service
           └─9540 /usr/bin/iqrf_startup /etc/iqrf-daemon/config.json
```

### Adjust your configuration 

Follow the guidance [here](https://github.com/iqrfsdk/iqrf-daemon/wiki/Configuration)

Configure mainly following components: 

- config.json 			(enable/disable daemon components)
- IqrfInterface.json    (configure your IQRF interface - SPI/CDC)
- TracerFile.json 		(configure your logging)
- MqttMessaging.json    (configure your MQTT brokers - you can enable second mqtt instance)
- BaseService.json      (see names of running instances of basic service - needed for scheduler)
- Scheduler.json        (configure your regural DPA tasks if any)  

and restart the service:

```Bash
sudo systemctl restart iqrf-daemon.service
```

### Content of iqrf-daemon (v0.1.2) package

```Bash
dpkg -L iqrf-daemon

/.
/usr
/usr/bin
/usr/bin/iqrfapp
/usr/bin/iqrf_startup
/usr/share
/usr/share/doc
/usr/share/doc/iqrf-daemon
/usr/share/doc/iqrf-daemon/copyright
/usr/share/doc/iqrf-daemon/changelog.gz
/lib
/lib/systemd
/lib/systemd/system
/lib/systemd/system/iqrf-daemon.service
/etc
/etc/iqrf-daemon
/etc/iqrf-daemon/UdpMessaging.json
/etc/iqrf-daemon/config.json
/etc/iqrf-daemon/MqMessaging.json
/etc/iqrf-daemon/TracerFile.json
/etc/iqrf-daemon/JsonSerializer.json
/etc/iqrf-daemon/IqrfInterface.json
/etc/iqrf-daemon/SimpleSerializer.json
/etc/iqrf-daemon/MqttMessaging.json
/etc/iqrf-daemon/Scheduler.json
/etc/iqrf-daemon/iqrfapp.json
/etc/iqrf-daemon/ClientServicePlain.json
```

### Content of libpaho.mqtt.c (v1.1.0) package

```Bash
dpkg -L libpaho.mqtt.c

/.
/usr
/usr/lib
/usr/lib/libpaho-mqtt3c.so.1.1.0
/usr/lib/libpaho-mqtt3as.so.1.1.0
/usr/lib/libpaho-mqtt3cs.so.1.1.0
/usr/lib/libpaho-mqtt3a.so.1.1.0
/usr/bin
/usr/bin/MQTTVersion
/usr/share
/usr/share/doc
/usr/share/doc/libpaho.mqtt.c
/usr/share/doc/libpaho.mqtt.c/copyright
/usr/share/doc/libpaho.mqtt.c/changelog.gz
```

## How to build iqrf-daemon from the source code

See the guidelines [here](BUILD.md)

## How to run iqrf-daemon in Docker container

See the guidelines [here](DOCKER.md)
