# Doofah

Doofah is a remote control emulator to be used in the Thunder ecosystem. It will consist of a Plugin communicating to an endpoint. The endpoint will contain the needed hardware to control the box. For now, we only focused on a BLE endpoint running on a cheap, widely available ESP32 microcontroller. We use the [M5Stack ATOM lite](https://docs.m5stack.com/en/core/atom_lite). IR should also be possible in the future. 



# Setup Endpoint
## Installation
1. Install [Visual Code Studio](https://code.visualstudio.com)
1. Install [PlatformIO](https://platformio.org/install)
1. Open the [Doofah-Endpoint](https://github.com/Metrological/ThunderDoofah/tree/main/Doofah-Endpoint) folder in Visual Code Studio.
1. Connect the ESP32 to your computer.
1. Setup the [upload_port](https://github.com/Metrological/ThunderDoofah/blob/main/Doofah-Endpoint/platformio.ini#L40) for your environment. 
1. Compile (:ballot_box_with_check:) and upload (:arrow_right:) the project (see the blue bar on the bottom of Visual Code Studio). 

* When you upload for the first time, please do a factory reset by pressing the button for more than 10 seconds. The led will blink red in this process  

## Enable verbose logging
There is an option to get some logging from the endpoint. An additional USB to TTL-Serial adapter is then required. 

1. Connect the USB to TTL-Serial to [these pins](https://github.com/Metrological/ThunderDoofah/blob/main/Doofah-Endpoint/platformio.ini#L51-L53) 
1. Connect the USB to TTL-Serial adapter to your computer.
1. Connect your ESP32 to your computer.
1. Setup the [monitor_port](https://github.com/Metrological/ThunderDoofah/blob/main/Doofah-Endpoint/platformio.ini#L41) for your environment. 
1. Compile and upload the project
1. Open the USB to TTL-Serial port on your computer
1. If you reset the ESP32, you should see something like this. 
    ```
    3 [lib/Storage/Storage.cpp:28 (0x3ffc3980) Allocate] 132 bytes avalaible on address 0
    12 [lib/Storage/Storage.cpp:28 (0x3ffc3980) Allocate] 19 bytes avalaible on address 134
    62 [lib/Storage/Storage.cpp:87 (0x3ffc3980) Begin] 
    69 [lib/Storage/Storage.cpp:89 (0x3ffc3980) Begin] Starting Storage with max 155 bytes
    77 [lib/Storage/Storage.cpp:115 (0x3ffc3980) DumpFlash] Flash[155]: '0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
    110 [lib/Contoller/Controller.cpp:79 (0x3ffc39a0) StartDevices] Starting 2 devices
    118 [lib/Storage/Storage.cpp:44 (0x3ffc3980) Read] No data
    433 [lib/Devices/BleKeyboardDevice.h:87 (0x3ffc23ec) Begin] Started BLE [94:b9:7e:8b:ee:f2]
    442 [lib/Storage/Storage.cpp:44 (0x3ffc3980) Read] No data
    448 [lib/Devices/IRKeyboardDevice.h:57 (0x3ffc23d8) Begin] Started IR [12]
    456 [src/main.cpp:242 setup] Starting endpoint build Wed Aug 10 13:22:05 2022
    463 [src/main.cpp:77 SendMessage] Sending 5 bytes with operation=0x80 result=0x00...
    471 [src/main.cpp:67 PrintMessage] SendMessage: message[8000000026]
    916 [src/main.cpp:77 SendMessage] Sending 5 bytes with operation=0x80 result=0x00...
    923 [src/main.cpp:67 PrintMessage] SendMessage: message[8000000026]
    ```  

# Setup Plugin

## Build
The plugin is a CMake-based project built like all other existing Thunder plugins 

Options:

1. ```PLUGIN_DOOFAH_AUTOSTART```: Automatically start the plugin; default: ```false```
2. ```PLUGIN_DOOFAH_CONNECTOR_CONFIG```: Custom config for the connector/serial port; default: ```""```)


## JSONRPC API

### list Devices
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.devices"
    }'
```
    
### Reboot Endpoint
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.reset",
        "params": {
            "device": "0x00"
        }
    }'
```

### Press Key
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.press",
        "params": {
            "device": "0x01",
            "code": "0x1233"
        }
    }'
```
### Release Key
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.release",
        "params": {
            "device": "0x01",
            "code": "0x1233"
        }
    }'
```

### Setup BLE device
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.setup",
        "params": {
            "device":"0x01",
            "configuration": {
                "type":"ble",
                "setup":{
                    "vid":"0x1234",
                    "pid":"0xabcd",
                    "name":"custom-doofer-name"
                }
            }
        }
    }'
```

### Clear BLE device
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Doofah' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Doofah.1.reset",
        "params": {
            "device": "0x01"
        }
    }'
```