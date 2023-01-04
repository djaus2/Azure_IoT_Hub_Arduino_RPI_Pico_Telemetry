# Azure IoT Hub Arduino Raspberry Pi Pico with Telemetry

## About
Adds some functioanlity, such as reading environment telemetry to [Azure/azure-sdk-for-c-arduino](https://github.com/Azure/azure-sdk-for-c-arduino) Rpi Pico example. This example has been added as a Pull Request to that repository which was pulled from the fork at [djaus2/azure-sdk-for-c-arduino](https://github.com/djaus2/azure-sdk-for-c-arduino). The first Sketch here (base) is that same proposed example Sketch and is in  ```examples/Azure_IoT_Hub_RPI_Pico``` in the djaus2 fork. (When merged in the origninal, it should be in the matching folder).

## Hardware
I'm using a Freenove Ultimate Starter Kit and Rpi Pico. See the ReadMe in the Base for further info.

## Sketches
- Azure_IoT_Hub_RPI_Pico_Base
  - The base version. Work through the ReadMe there to get the Pico up and running with Azure IoT Hub
  - It also includes an addendum for porting the ESP8266 example to the Pico, from the original repository.
  - Only sends an incremented count as telemetry.
- Azure_IoT_Hub_RPI_Pico_DHT11
  - Reads a DHT11 sensor and sends Temperature and Pressure as Telemetry.
  - Makes use of formal generation of Json string.
  - This is a template for other sensors.
- Azure IoT Hub RPI Pico LDR
  - Reading light intensity using a Light Dependent Resistor (LDR)  as per the DHT11 template.
- Azure_IoT_Hub_RPI_Pico_Thermistor
  - A Thermistor temperature sensor as per the DHT11 template.
  - Circuit and code quite similar to the LDR.
 - Azure_IoT_Hub_RPI_Pico_BME280 *Coming*
 - Azure IoT Hub RPI Pico LDR with Cload to Device Messages and Commands
   - Messages: All versions will display Cloud to Device Messages to it. Unchanged
   - This version will interpret a number of Cloud to Device Commands (Methods) and run them. Added
     - Lacks ability to send acknowledgment back to the cloud.
 
 ## 2Do
 - Handle CD Messages, Methods, Properties etc.

## Setting up the IoT Hub

I have another repository that when cloned and opened in VS Code provides a menu driven context for creating 
and managing an Azure IoT Hub and releated entities. See:

[Azure IoT Hub PowerShell Scripts](https://github.com/djaus2/az-iothub-ps)

- Get an Azure Subscription (You can get a free one for a month or so).
- Clone the above repository and open the PS folder in VS Code and from a terminal run:

```
./get-iothub
```

> **New:** Can generate C Header File Device connectivity for Azure SDK for C Arduino:  
In get-iothub, Menu 6 then 1. Info at bottom.

![get-iothub.ps1](./get-iothub.png)

= Select the subscription and login
- Create a new Group
- Create a new IoT Hub
- Create a new Device
- Get the Connectivity details *(Environment Variables)**
  - Menu OPtion 6 then 1. Info at bottom.  
Copy and paste into header file

