---
page_type: sample
description: Connecting a RPI Pico W as an Arduino device, to Azure IoT using the Azure SDK for Embedded C. Based upon the ESP8256 version.
languages:
- c
products:
- azure-iot
- azure-iot-pnp
- azure-iot-dps
- azure-iot-hub
---

# Azure IoT Hub RPI Pico DHT11

-   [Getting Started](#Getting-Started)
    
-   [Setup for the DHT11](#Setup-for-the-DHT11)

## Getting Started

- Get the Pico sending some simulated data as per the Base project ReadMe.

## Setup for the DHT11
- Setup the WiFi and Azure IoT Hub connection in iot_configs.h as per the Base._ 
- Add this DHT11 library:
  - As previuosly mentioned, the FrenoveStarter Kit for the Rpi Pico was used:
    - Clone the repoitory https://github.com/Freenove/Freenove_Ultimate_Starter_Kit_for_Raspberry_Pi_Pico
  - Open Arduino>Sketch>Include Library>Add .ZIP Library...
  - Starting at the root of the clone locally, select the provided```Freenove_Ultimate_Starter_Kit_for_Raspberry_Pi_Pico\C\Libraries\DHT.zip```。
    - Ps: You might like to run Sketch_25.1_Temperature_and_Humidity_Sensor from ```Freenove_Ultimate_Starter_Kit_for_Raspberry_Pi_Pico-master\C\Sketches\Sketch_25.1_Temperature_and_Humidity_Sensor``` in the cloned contents first to test the DHT11.
- Add The ArduinoJson Library:
  - Ref: https://arduinojson.org/
  - In the Arduino Library Manager search for “ArduinoJson”
  - Select and install.
- Verify the code

## Running
- Upload then switch to the Serial Monitor.
- You should see something like:
```
.........WiFi connected, IP address: 192.168.0.14
Setting time using SNTP......done!
Current time: Sun Jan  1 04:30:55 2023
Client ID: PicoDev137
Username: PicoHub137.azure-devices.net/PicoDev137/?api-version=2020-09-30&DeviceClientType=c%2F1.5.0-beta.1(ard;rpipico)
MQTT connecting ... connected.
11352 RPI Pico (Arduino) Sending telemetry . . . OK
14052 RPI Pico (Arduino) Sending telemetry . . . OK
16947 RPI Pico (Arduino) Sending telemetry . . . OK
```

## Monitor Telemetry

### In a Terminal
- In a desktop terminal context that has AzureCli with the IoT Extensiomn installed (See Base ReadMe):  
```
az iot hub monitor-events --login <your Azure IoT Hub owner connection string in quotes> --device-id <your device id>
```

```
{
    "event": {
        "origin": "PicoDev137",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{\"msgCount\":163,\"temp\":31.2,\"humidity\":45}"
    }
}
{
    "event": {
        "origin": "PicoDev137",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{\"msgCount\":164,\"temp\":31.2,\"humidity\":45}"
    }
}
{
    "event": {
        "origin": "PicoDev137",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{\"msgCount\":165,\"temp\":31.2,\"humidity\":44}"
    }
}
```

### In VS Code
- Add the Azure IoT Hub Extension
- Add The IoT Hub [Select and IoT Hub] and follow the directions.
  - You need to select the Subscription 
  - You will be prompted for connefction details
  - Then select the Hub
- In the left pane select the hub then the device.
- Right click on that and select "Start Monitoring Built-In Endpoint"

```
[IoTHubMonitor] [3:40:22 PM] Message received from [PicoDev137]:
{
  "msgCount": 216,
  "temp": 31.2,
  "humidity": 44
}
[IoTHubMonitor] [3:40:25 PM] Message received from [PicoDev137]:
{
  "msgCount": 217,
  "temp": 31.2,
  "humidity": 44
}
[IoTHubMonitor] [3:40:28 PM] Message received from [PicoDev137]:
{
  "msgCount": 218,
  "temp": 31.2,
  "humidity": 44
}
```