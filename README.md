# Azure IoT Hub Arduino Raspberry Pi Pico with Telemetry

## About
Adds some functioanlity, such as reading environment telemetry to [Azure/azure-sdk-for-c-arduino](https://github.com/Azure/azure-sdk-for-c-arduino) Rpi Pico example. This example has been added as a Pull Request to that repository which was pulled from the fork at [djaus2/azure-sdk-for-c-arduino](https://github.com/djaus2/azure-sdk-for-c-arduino). The first Sketch here (base) is that same proposed example Sketch and is in  ```examples/Azure_IoT_Hub_RPI_Pico``` in the djaus2 fork. (When merged in the origninal, it should be in the matching folder).

## Hardware
I'm using a Freenove Basic Starter Kit and Rpi Pico. See the ReadMe in the Base for further info.

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
  - Reading light intensity using a Light Dependent Resistor (LDR).
- Azure_IoT_Hub_RPI_Pico_Thermistor
  - A Thermistor temperature sensor as per the DHT11 template.
  - Circuit and code quite similar to the LDR


