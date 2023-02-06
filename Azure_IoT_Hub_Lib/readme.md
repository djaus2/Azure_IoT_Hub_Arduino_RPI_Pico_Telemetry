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
- Note can set level of messaging 0 to 5 in ```az_local_msglevels.h``` 
  - 0 only shows top level messages.

## Sample output
At startup Level=0

```
================================================================================
Set Inital Device Properties on Device:
================================================================================
 - Default properties set.
================================================================================

================================================================================
Save Device Properties on Device:
================================================================================
{
  "IsRunning": false,
  "TelemetryFrequencyMilliseconds": 10000,
  "MethodsSubscribed": false,
  "CDMessagesSubscribed": false,
  "LEDIsOn": false,
  "fanOn": false
}
 - Saved properties.
================================================================================

================================================================================
Load Device Properties from Device:
================================================================================
 - Loaded properties.
================================================================================

================================================================================
Client requesting device twin document from service:
================================================================================
 - OK Published the Twin Document request. 
 - Request done
================================================================================

================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/twin/res/200
  Got Payload

================================================================================
Set Desired Properties:
================================================================================
{
  "desired": {
    "TelemetryFrequencyMilliseconds": 111,
    "IsRunning": false,
    "fanOn": false,
    "$version": 28
  },
  "reported": {
    "IsRunning": true,
    "TelemetryFrequencyMilliseconds": 111,
    "MethodsSubscribed": false,
    "CDMessagesSubscribed": false,
    "LEDIsOn": false,
    "$version": 306
  }
}

================================================================================
Save Device Properties on Device:
================================================================================
{
  "IsRunning": false,
  "TelemetryFrequencyMilliseconds": 111,
  "MethodsSubscribed": false,
  "CDMessagesSubscribed": false,
  "LEDIsOn": false,
  "fanOn": false
}
 - Saved properties.
================================================================================
 - Desired Properties Set
================================================================================
 - END Callback
================================================================================

================================================================================
Reporting Device Properties to Hub:
================================================================================
 - Reported
================================================================================

================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/twin/res/204
No Payload
 - END Callback
===============================================================================


================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/methods/POST/start  <--start Method Call
  Got Payload

================================================================================
Save Device Properties on Device:
================================================================================
{
  "IsRunning": true,
  "TelemetryFrequencyMilliseconds": 111,
  "MethodsSubscribed": false,
  "CDMessagesSubscribed": false,
  "LEDIsOn": false,
  "fanOn": false
}
 - Saved properties.
================================================================================
 - END Callback
================================================================================
337871 RPI Pico (Arduino) Sending telemetry . . . devices/Pico10Dev/messages/events/StatD
{"LightIntensity":89,"msgCount":0}
OK
338640 RPI Pico (Arduino) Sending telemetry . . . devices/Pico10Dev/messages/events/StatD
{"LightIntensity":89,"msgCount":1}
OK
```
