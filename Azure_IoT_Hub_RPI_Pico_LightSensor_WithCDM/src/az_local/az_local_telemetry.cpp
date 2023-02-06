#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local_msglevels.h"
#include "az_local.h"


#include <string.h>

///
// Generate payload
///
char * generateTelemetryPayload(uint32_t telemetry_send_count, const char * propertyName, int value)
{   
    DynamicJsonDocument doc(1024);
    char jsonStr[1024];

    doc[propertyName] = value;
    doc["msgCount"] = telemetry_send_count;
    serializeJson(doc, jsonStr);
    az_span temp_span = az_span_create_from_str(jsonStr);
    az_span_to_str((char*)telemetry_payload, sizeof(telemetry_payload), temp_span);

    return (char*)telemetry_payload;
}

///
// Add a (sample) property to telemetry message
///

az_iot_message_properties * GetProperties(int value)
{
  uint32_t msgLength;
  az_result az_result;
  if (value>100)
  {
    //From: https://github.com/Azure/azure-sdk-for-c  Issue #1471
    msgLength = (uint32_t)strlen(NO_WARNING);
    az_span string = AZ_SPAN_LITERAL_FROM_STR(NO_WARNING);
    uint8_t a[64];
    az_span s = AZ_SPAN_FROM_BUFFER(a);
    az_span_copy(s, string);
    az_result = az_iot_message_properties_init(&properties, s, msgLength);
  }
  else
  {
     //From: https://github.com/Azure/azure-sdk-for-c  Issue #1471
    msgLength = (uint32_t)strlen(WARNING);
    az_span string = AZ_SPAN_LITERAL_FROM_STR(WARNING);
    uint8_t a[64];
    az_span s = AZ_SPAN_FROM_BUFFER(a);
    az_span_copy(s, string);
    az_result = az_iot_message_properties_init(&properties, s, msgLength);
  }
  if(az_result = AZ_OK )
    return &properties;
  else
    return NULL;
} 