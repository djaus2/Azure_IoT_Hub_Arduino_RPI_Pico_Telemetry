// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Wifi
#define IOT_CONFIG_WIFI_SSID "APQLZM"
#define IOT_CONFIG_WIFI_PASSWORD "silly1371"

// Azure IoT
#define IOT_CONFIG_IOTHUB_FQDN "[your Azure IoT host name].azure-devices.net"
#define IOT_CONFIG_DEVICE_ID "Device ID"
#define IOT_CONFIG_DEVICE_KEY "Device Key"

#define PIN_ADC0   26



//Nb: By default, Acknoweledgement is not required for CD Messages. So use none
#define ACK_MODE full
#define CD_MESSAGE_OUTCOME success

// Publish 1 message every 10 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 10000

struct Properties {
	unsigned long TelemetryFrequencyMilliseconds = TELEMETRY_FREQUENCY_MILLISECS;
	bool IsRunning = false;
	bool LEDIsOn = false;
	bool fanOn = false;
};

