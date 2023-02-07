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

## Sample output at startup
At startup with``` _MSGLevel_=0 and _USEOUTLINING_```, in az_local_msglevels.h

### 1. Initial WiFi and MQTT Setup
```
Connecting to WIFI SSID APQLZM
............WiFi connected, IP address: 192.168.0.2
Setting time using SNTP......done!
Current time: Tue Feb  7 12:02:01 2023
                                      Client ID: Pico12Dev
Username: Pico12Hub.azure-devices.net/Pico12Dev/?api-version=2020-09-30&DeviceClientType=c%2F1.5.0-beta.1(ard;rpipico)
MQTT connecting ... connected.
```

### 2. Setup device with local property set
```
================================================================================
Set Inital Device Properties on Device:
================================================================================
 - Default properties set.
================================================================================
{
  "IsRunning": false,
  "TelemetryFrequencyMilliseconds": 10000,
  "MethodsSubscribed": false,
  "CDMessagesSubscribed": false,
  "LEDIsOn": false,
  "fanOn": false
}
```

### 3. Request Twin document from the IoT Hub
```
================================================================================
Client requesting device twin document from service:
================================================================================
 - OK Published the Twin Document request.
 - Request done
================================================================================
```

### 4. Got Twin document, set Desired properties
```
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
    "IsRunning": false,
    "TelemetryFrequencyMilliseconds": 4567,
    "$version": 30
  },
  "reported": {
    "IsRunning": false,
    "TelemetryFrequencyMilliseconds": 4567,
    "MethodsSubscribed": false,
    "CDMessagesSubscribed": false,
    "LEDIsOn": false,
    "$version": 161
  }
}{
  "IsRunning": false,
  "TelemetryFrequencyMilliseconds": 4567,
  "MethodsSubscribed": false,
  "CDMessagesSubscribed": false,
  "LEDIsOn": false,
  "fanOn": false
} - Desired Properties Set
================================================================================
 - END Callback
================================================================================
```

### 5. Report back properties
```
================================================================================
Reporting Device Properties to Hub:
================================================================================
 - Reported
================================================================================
```

### 6. Get acknowledgement back from Hub
- One acknowledgement for each property.
- Reported properties were sent individually.
```
================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/twin/res/204
No Payload
 - END Callback
================================================================================

================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/twin/res/204
No Payload
 - END Callback
================================================================================

================================================================================
Got IoT Hub Doc-Message-Method-Response
================================================================================
Got Topic: $iothub/twin/res/204
No Payload
 - END Callback
================================================================================

```
