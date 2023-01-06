// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Wifi
#define IOT_CONFIG_WIFI_SSID "APQLZM"
#define IOT_CONFIG_WIFI_PASSWORD "silly1371"

// Azure IoT
#define IOT_CONFIG_IOTHUB_FQDN "[your Azure IoT host name].azure-devices.net"
#define IOT_CONFIG_DEVICE_ID "Device ID"
#define IOT_CONFIG_DEVICE_KEY "Device Key"

// Publish 1 message every 2 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 10000

#define WARNING "Status=Warning Value less than 100"
#define NO_WARNING "Status=NO Warning Value greater 100"

//Used in AZ_NODISCARD az_result az_span_relaxed_atou32():
// Get char digits into an integer from the source as an az_span until char is not a digit
// Digits are HEX as per _az_NUMBER_OF_DECIMAL_VALUES
#define _az_NUMBER_OF_DECIMAL_VALUES 16

