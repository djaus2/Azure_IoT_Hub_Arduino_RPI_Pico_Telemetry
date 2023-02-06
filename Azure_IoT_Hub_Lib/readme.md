# The az_local Library

## About
This is the library as used as source code in the **Azure_IoT_Hub_RPI_Pico_LightSensor_WithCDM** Sketch. 
The base functionality is only Telemetry and CD Messages. This adds Direct Methods and Twinning.

## How to use this library

- Copy the src folder into an Arduino Sketch
- Make reference to az_local.h:
```
#include "src/az_local/az_local.h"
```
- Need to install the **Azure SDK for Embedded C**
  - See The ReadMe in the Azure_IoT_Hub_RPI_Pico_Base Sketch in this repository.
