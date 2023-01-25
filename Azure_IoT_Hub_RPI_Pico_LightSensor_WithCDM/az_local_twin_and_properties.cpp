#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local.h"

#include "iot_configs.h"
#include <string.h>

void InitProperties()
{
  Serial.println();
  Serial.println("Inital Device Properties:");  
  // Clear the storage
  memset(PropsJson,0,strlen(PropsJson));
  Dev_Properties.IsRunning =false;
  Dev_Properties.TelemetryFrequencyMilliseconds = TELEMETRY_FREQUENCY_MILLISECS;
  Dev_Properties.MethodsSubscribed = false;
  Dev_Properties.CDMessagesSubscribed = false;
  Dev_Properties.LEDIsOn = false;

  DynamicJsonDocument doc(512);
  doc["IsRunning"] = Dev_Properties.IsRunning;
  doc["TelemetryFrequencyMilliseconds"]= Dev_Properties.TelemetryFrequencyMilliseconds;
  doc["MethodsSubscribed"]=Dev_Properties.MethodsSubscribed;
  doc["CDMessagesSubscribed"]=Dev_Properties.CDMessagesSubscribed;
  doc["LEDIsOn"]=Dev_Properties.LEDIsOn;
  serializeJson(doc, PropsJson);
  //Serial.println(PropsJson);
  serializeJsonPretty(doc, Serial);
  Serial.println();
  Serial.println();
}

void get_device_twin_document(void)
{
  int rc;
  Serial.println("Client requesting device twin document from service.");

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
        "Failed to get the Twin Document topic: az_result return code ");
    Serial.println(rc,HEX);
    exit(rc);
  }
  bool res;
  // Publish the twin document request.
  res = mqtt_client.publish(
       twin_document_topic_buffer, NULL, 0,  NULL);
  if (!res)
  {
    Serial.println("FAILED to publish the Twin Document request. ");
    exit(99);
  }
  else
  {
    Serial.println("OK Published the Twin Document request. ");    
  }
}

void SetProperties( char * payload)
{
  DynamicJsonDocument doc(512);;
  Serial.println();
  Serial.println("Set Desired Properties");
  deserializeJson(doc, payload);
  Serial.println("Payload: Json pretty print:");
  serializeJsonPretty(doc, Serial);
  Serial.println("###################");
  char desired[128];
    //serializeJson(Props, PropsJson); 
   JsonObject roota = doc.as<JsonObject>();
   if (doc.containsKey("desired")) 
   {
     Serial.println("Desired Properties:");
     JsonObject jv = roota["desired"];
     serializeJson(jv, PropsJson);
     for (JsonPair kv : jv) {
        const char * key = kv.key().c_str();
        JsonVariant jvv = kv.value();
        if (strcmp("components",key)==0)
        {
            Serial.println("Has Components");
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
            Serial.println("Has Modules");
        }
        else
        {
          Serial.print(key);
          Serial.print(": ");
          if (jvv.is<String>())
          {
            Serial.print("<String>");
            Serial.println( jvv.as<String>().c_str());
          }
          else  if (jvv.is<bool>())
          {
            Serial.print("<bool>");
            Serial.println( jvv.as<bool>());
          }
          else  if (jvv.is<int>())
          {
            Serial.print("<int>");
            Serial.println( jvv.as<int>());
          }
          else  if (jvv.is<float>())
          {
            Serial.print("<float>");
            Serial.println( jvv.as<float>());
          }
          else  if (jvv.is<double>())
          {
            Serial.print("<double>");
            Serial.println( jvv.as<double>());
          }
          else  if (jvv.is<unsigned char>())
          {
            Serial.print("<char>");
            Serial.println( jvv.as<unsigned char>());
          }
          else  
          {
            Serial.print("< ??? >");
            Serial.println( jvv.as<String>().c_str());
          }
        }
      }
    }

/*
    for (JsonPair kv : roota) {
    Serial.print(kv.key().c_str());
    Serial.println(": ");
    Serial.println(kv.value().as<String>());
    if (strncmp("desired",  kv.key().c_str(), strlen("desired"))==0)
    {
      Serial.print("GOT desired: ");
      
      strcpy(desired,kv.value().as<String>().c_str());
      Serial.println(desired);
    }
  }*/
 Serial.println("###################");  
 /*
  Serial.println("Saving Properties");
  DynamicJsonDocument Props(512);
  JsonObject rootzw = messageReceivedDoc.as<JsonObject>();
  const char * msg = rootzw["desired"];
  Serial.println(msg);
  deserializeJson(Props,msg); 


  Serial.println("Properties: Json pretty print:");
  serializeJsonPretty(Props, Serial);
  Serial.println();
  JsonObject rootz = Props.as<JsonObject>();
  Serial.println("... As Json Object:");
  for (JsonPair kv : rootz) {
    Serial.print(kv.key().c_str());
    Serial.print(": ");
    Serial.println(kv.value().as<String>());
  }*/
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

void send_reported_property(const char * propertyName, int32_t propertyValue)
{
  int rc;

  Serial.println("Client sending reported property to service.");

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
  build_reported_property_int(propertyName,propertyValue,reported_property_payload, &reported_property_payload);

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
  Serial.println("Client published the Twin Patch reported property message.");
  Serial.print("Sent Property: ");
  Serial.print(propertyName);
  Serial.print("<int> Value: ");
  Serial.println(propertyValue);

}

void UpdateProperties()
{

}