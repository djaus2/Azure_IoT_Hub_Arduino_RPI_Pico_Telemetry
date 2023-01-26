#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local.h"

#include "iot_configs.h"
#include <string.h> 

// Set properties initally at startup to some default values
// Would be useful to get these properties from a file.
// But will get them from the IoT Hub
void InitProperties()
{
    PRINT_BEGIN("Set Inital Device Properties on Device:");
    // Clear the storage 
    memset(PropsJson, 0, strlen(PropsJson));
    Dev_Properties.IsRunning = false;
    Dev_Properties.TelemetryFrequencyMilliseconds = TELEMETRY_FREQUENCY_MILLISECS;
    Dev_Properties.MethodsSubscribed = false;
    Dev_Properties.CDMessagesSubscribed = false;
    Dev_Properties.LEDIsOn = false;
    SaveProperies();
    PRINT_END("Inital Device Properties:");
}

void SaveProperies()
{
  PRINT_BEGIN("Save Device Properties on Device:");
  DynamicJsonDocument doc(512);
  doc["IsRunning"] = Dev_Properties.IsRunning;
  doc["TelemetryFrequencyMilliseconds"]= Dev_Properties.TelemetryFrequencyMilliseconds;
  doc["MethodsSubscribed"]=Dev_Properties.MethodsSubscribed;
  doc["CDMessagesSubscribed"]=Dev_Properties.CDMessagesSubscribed;
  doc["LEDIsOn"]=Dev_Properties.LEDIsOn;
  // Save properties as Json string to storage. Relying on DynamicJsonDocument can lead to memmory leaks.
  serializeJson(doc, PropsJson);
  serializeJsonPretty(doc, Serial);
  Serial.println();
  PRINT_END("Save Device Properties on Device:");
}

void ReportProperties()
{
    PRINT_BEGIN("Reporting Device Properties to Hub:")
    send_reported_property("IsRunning", (byte *)&Dev_Properties.IsRunning, sizeof(Dev_Properties.IsRunning), DT_BOOL);
    send_reported_property("TelemetryFrequencyMilliseconds", (byte*)&Dev_Properties.TelemetryFrequencyMilliseconds, sizeof(Dev_Properties.TelemetryFrequencyMilliseconds), DT_INT);
    send_reported_property("MethodsSubscribed", (byte*)&Dev_Properties.MethodsSubscribed , sizeof(Dev_Properties.MethodsSubscribed), DT_BOOL);
    send_reported_property("CDMessagesSubscribed", (byte*)&Dev_Properties.CDMessagesSubscribed, sizeof(Dev_Properties.CDMessagesSubscribed), DT_BOOL);
    send_reported_property("LEDIsOn", (byte*)&Dev_Properties.LEDIsOn, sizeof(Dev_Properties.LEDIsOn), DT_BOOL);
    PRINT_END("Reporting Device Properties to Hub")
}

void get_device_twin_document(void)
{
  int rc;
  PRINT_BEGIN("Client requesting device twin document from service:");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  rc = az_iot_hub_client_twin_document_get_publish_topic(
      &client,
      twin_document_topic_request_id,
      twin_document_topic_buffer,
      sizeof(twin_document_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    Serial.print(
        " - Failed to get the Twin Document topic: az_result return code ");
    Serial.println(rc,HEX);
    exit(rc);
  }
  bool res;
  // Publish the twin document request.
  res = mqtt_client.publish(
       twin_document_topic_buffer, NULL, 0,  NULL);
  if (!res)
  {
    Serial.println(" - FAILED to publish the Twin Document request. ");
    exit(99);
  }
  else
  {
    Serial.println(" - OK Published the Twin Document request. ");    
  }
  PRINT_END("Client requesting device twin document from service:");
}

void SetProperties( char * payload)
{
  DynamicJsonDocument doc(512);;
  PRINT_BEGIN("Set Desired Properties:");
  deserializeJson(doc, payload);
  PRINT_BEGIN_SUB("Payload: Json pretty print:");
  serializeJsonPretty(doc, Serial);
  Serial.println();
  PRINT_END_SUB("Payload: Json pretty print:");
  char desired[128];
    //serializeJson(Props, PropsJson); 
   JsonObject roota = doc.as<JsonObject>();
   if (doc.containsKey("desired")) 
   {
     PRINT_BEGIN_SUB("Desired Properties:");
     JsonObject jv = roota["desired"];
     serializeJson(jv, PropsJson);
     for (JsonPair kv : jv) {
        const char * key = kv.key().c_str();
        JsonVariant jvv = kv.value();
        if (strcmp("components",key)==0)
        {
            Serial.println("- Has Components");
            JsonObject jvComponents = jvv.as<JsonObject>();
            size_t NumberOfElements = sizeof(components)/sizeof(components[0]);
            for (int i=0;i< NumberOfElements; i++)
            {
              JsonVariant jvComponent = jvComponents[components[i]];
              JsonObject jvComponentObj = jvComponent.as<JsonObject>();
              Serial.print('\t');
              Serial.print(components[i]);
              Serial.println(":");
              for (JsonPair kv : jvComponentObj)
              {
                Serial.print('\t');
                Serial.print('\t');
                Serial.print( kv.key().c_str());
                Serial.print(": ");
                Serial.println(kv.value().as<String>().c_str());
              }
            }
        }
        else  if (strcmp("modules",key)==0)
        {
            Serial.println(" - Has Modules");
        }
        else
        {
            uint32_t iVal;
            bool bVal;
            double dVal;
            String sVal;
            Serial.print(" - ");
            Serial.print(key);
            Serial.print(": ");

            if (jvv.is<String>())
            {
                sVal = jvv.as<String>();
                Serial.print("<String>");
                Serial.println(jvv.as<String>().c_str());
            }
            else  if (jvv.is<bool>())
            {
                bVal = jvv.as<bool>();
                Serial.print("<bool>");
                Serial.println(jvv.as<bool>());
            }
            else  if (jvv.is<int>())
            {
                iVal = jvv.as<int>();
                Serial.print("<int>");
                Serial.println(jvv.as<int>());
            }
            else  if (jvv.is<float>())
            {
                dVal = jvv.as<double>();
                Serial.print("<float>");
                Serial.println(jvv.as<float>());
            }
            else  if (jvv.is<double>())
            {
                dVal = jvv.as<double>();
                Serial.print("<double>");
                Serial.println(jvv.as<double>());
            }
            else  if (jvv.is<unsigned char>())
            {
                iVal = (uint8_t)jvv.is<unsigned char>();
                Serial.print("<char>");
                Serial.println(jvv.as<unsigned char>());
            }
            else
            {
                Serial.print("< ??? >");
                Serial.println(jvv.as<String>().c_str());
            }

            // Following assumes correct type of value. Should check.
            if (strcmp(key, "IsRunning") == 0)
            {
                Dev_Properties.IsRunning = bVal;
            }
            else if (strcmp(key, "TelemetryFrequencyMilliseconds") == 0)
            {
                Dev_Properties.TelemetryFrequencyMilliseconds = iVal;
            }
            else if (strcmp(key, "MethodsSubscribed") == 0)
            {
                Dev_Properties.MethodsSubscribed = bVal;
            }
            else if (strcmp(key, "CDMessagesSubscribed") == 0)
            {
                Dev_Properties.CDMessagesSubscribed = bVal;
            }
            else if (strcmp(key, "LEDIsOn") == 0)
            {
                Dev_Properties.LEDIsOn = bVal;
            }
        }
      }
    }
   SaveProperies();
   PRINT_END("Set Desired Properties:");
}

#define IOT_SAMPLE_EXIT_IF_AZ_FAILED(A,B) if(allOK){ int rc = A; if (az_result_failed(rc)){Serial.print("Error - "); Serial.println(log); allOK = false;}}

void build_reported_property_int(
    const char * desired_property_name,
    int32_t device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload)
{

  az_span az_desired_property_name =  az_span_create_from_str((char *)desired_property_name);
  char const* const log = "Failed to build reported property payload";

  az_json_writer jw;
  bool allOK=true;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, reported_property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, az_desired_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_int32(&jw, device_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  if (allOK)
  {
    *out_reported_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  }
}

void build_reported_property_double(
    const char * desired_property_name,
    double device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload)
{

  az_span az_desired_property_name =  az_span_create_from_str((char *)desired_property_name);
  char const* const log = "Failed to build reported property payload";

  az_json_writer jw;
  bool allOK=true;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, reported_property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, az_desired_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_double(&jw, device_property_value, NUM_REPORTED_DOUBLE_FRACTIONAL_DIGITS), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  if (allOK)
  {
    *out_reported_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  }
}

void build_reported_property_bool(
    const char * desired_property_name,
    bool device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload)
{

  az_span az_desired_property_name =  az_span_create_from_str((char *)desired_property_name);
  char const* const log = "Failed to build reported property payload";

  az_json_writer jw;
  bool allOK=true;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, reported_property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, az_desired_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_bool(&jw, device_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  if (allOK)
  {
    *out_reported_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  }
}


void build_reported_property_null(
    const char * desired_property_name,
    az_span reported_property_payload,
    az_span* out_reported_property_payload)
{

  az_span az_desired_property_name =  az_span_create_from_str((char *)desired_property_name);
  char const* const log = "Failed to build reported property payload";

  az_json_writer jw;
  bool allOK=true;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, reported_property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, az_desired_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_null(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  if (allOK)
  {
    *out_reported_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  }
}

void send_reported_property(const char* propertyName, byte * propertyValue, uint8_t propertySize, CD_TWIN_PROPERTY_DATA_TYPE propertyType)
{
    int rc;

    PRINT_BEGIN_SUB("Client sending reported property to service.");

    // Get the Twin Patch topic to publish a reported property update.
    char twin_patch_topic_buffer[128];

    rc = az_iot_hub_client_twin_patch_get_publish_topic(
        &client,
        twin_patch_topic_request_id,
        twin_patch_topic_buffer,
        sizeof(twin_patch_topic_buffer),
        NULL);
    if (az_result_failed(rc))
    {
        Serial.print("ERROR Failed to get the Twin Patch topic: az_result return code :");
        Serial.println(rc, HEX);
        return;
    }

    // Build the updated reported property message.
    char reported_property_payload_buffer[128];
    az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);
    switch (propertyType)
    {
    case DT_NULL:
        build_reported_property_null(propertyName,  reported_property_payload, &reported_property_payload);
        break;
    case DT_INT:
        build_reported_property_int(propertyName,* ((int *)propertyValue), reported_property_payload, &reported_property_payload);
        break;
    case DT_BOOL:
        build_reported_property_bool(propertyName,* ((bool *) propertyValue), reported_property_payload, &reported_property_payload);
        break;
    case DT_DOUBLE:
        build_reported_property_double(propertyName,* ((double *)propertyValue), reported_property_payload, &reported_property_payload);
        break;
    default:
        Serial.println("Data Type DT_ not yet implemented");
        return;
        break;
    }
  bool res;
  // Publish the twin document request.
  res = mqtt_client.publish(
       twin_patch_topic_buffer,
        az_span_ptr(reported_property_payload), 
        az_span_size(reported_property_payload),  
        false
  );
  if (!res)
  {
    Serial.print(
        "Failed to publish the Twin Patch reported property update: MQTTClient return code: ");
        Serial.println(rc);
    return;
  }
  Serial.println(" - Client published the Twin reported property message.");
    Serial.print("  - Sent Property: ");
  Serial.println(propertyName);
  Serial.print("    - ");

  switch (propertyType)
  {
  case DT_NULL:
      Serial.print("Value: ");
      Serial.println("NULL");
      break;
  case DT_INT:
      Serial.print("<int> Value: ");
      Serial.println(*((int *)propertyValue));     
      break;
  case DT_BOOL:
      Serial.print("<bool> Value: ");
      Serial.println(* ((bool *)propertyValue));  
      break;
  case DT_DOUBLE:
      Serial.print("<double> Value: ");
      Serial.println(* ((double *)propertyValue));      
      break;
  default:
      Serial.println("Data Type DT_ not yet implemented");
      return;
      break;
  }
}

void UpdateProperties()
{

}