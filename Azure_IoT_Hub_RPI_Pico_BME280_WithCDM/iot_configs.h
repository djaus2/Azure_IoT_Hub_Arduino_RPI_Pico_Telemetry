// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#define IOT_CONFIG_WIFI_SSID "SSID"
#define IOT_CONFIG_WIFI_PASSWORD "PWD"

// Azure IoT
#define IOT_CONFIG_IOTHUB_FQDN "[your Azure IoT host name].azure-devices.net"
#define IOT_CONFIG_DEVICE_ID "Device ID"
#define IOT_CONFIG_DEVICE_KEY "Device Key"


// Publish 1 message every 2 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 5000

#define TELEMETRY_NAMES "temperature,humidity,pressure"

enum telemetryOrd {otemp,ohum,opres};
