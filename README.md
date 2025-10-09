# Garage door opener
## Running the project

<b>Update git submodules</b>
```
git submodule update --init --recursive
```

<b>Create a file "include/network_info.h"</b>
```
#ifndef NETWORK_INFO_H
#define  NETWORK_INFO_H

#define  SSID  <YOUR WI-FI SSID>
#define  PW  <YOUR WI-FI PASSWORD>

#define  HOSTNAME  <YOUR DEVICE HOSTNAME RUNNING MQTT SERVER>
#define  PORT  <PORT THAT MQTT SERVER IS RUNNING ON>

#endif
```

<b>Run mqtt server</b><br>
Copy your mosquitto.conf into config/mosquitto.conf and run:
```
/* AS ADMIN */
docker compose up -d
```

<b>Build and flash<b>
```
make
```