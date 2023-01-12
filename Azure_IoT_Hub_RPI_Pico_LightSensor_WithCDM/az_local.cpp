
#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local.h"
#include "iot_configs.h"


bool IsRunning;
bool LEDIsOn;
unsigned long TelemetryFrequencyMilliseconds;
char telemetry_topic[128];
uint8_t telemetry_payload[100]; 
az_iot_message_properties properties;

//Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-c2d
enum CD_MESSAGE_ACKS {none,full,postive,negative} Ack_Mode = ACK_MODE;
const char * CD_Message_Ack[] = {"none", "full", "postive", "negative"};
//Ref: https://learn.microsoft.com/en-us/dotnet/api/microsoft.azure.devices.feedbackstatuscode?view=azure-dotnet
enum CD_MESSAGE_OUTCOMES {success, expired, deliveryCountExceeded, rejected, purged} MsgOutcome = CD_MESSAGE_OUTCOME;
const char * CD_Message_Outcome[] = {"Success", "Expired", "Delivery Count Exceeded", "Rejected", "Purged"};


char * methodResponseBuffer;
DynamicJsonDocument methodResponseDoc(64);
// Called by receivedCallback() if call is a CD Method call after the method is actioned
// Generate the payload to send back
char* get_Method_Response(
  az_span az_span_id, 
  char * method, 
  char * parameter, 
  uint32_t id,
  uint16_t status 
  )
{
  char jsonResponseStr[100];
  methodResponseBuffer = (char *)malloc(100);
  az_result rc = az_iot_hub_client_methods_response_get_publish_topic(
    &client, //&hub_client,
    az_span_id,
    status,
    methodResponseBuffer,
    100,
    NULL);
    if (az_result_failed(rc))
  {
    Serial.println("Falied: az_iot_hub_client_methods_response_get_publish_topic");
  }
  methodResponseDoc["request_id"] = id;
  methodResponseDoc["mothod"] = method;
  methodResponseDoc["parameter"] = parameter;
  serializeJson(methodResponseDoc, jsonResponseStr);
  az_span temp_span = az_span_create_from_str(jsonResponseStr);
  az_span_to_str((char *)telemetry_payload, sizeof(telemetry_payload), temp_span);
  return (char*)telemetry_payload;
}

/* Suggested code for this from 
   https://github.com/Azure/azure-sdk-for-c/blob/d0ae29572be3ecf4ff272b11c62f448649b63268/sdk/samples/iot/paho_iot_hub_twin_sample.c
   (static void send_method_response(
    az_iot_hub_client_method_request const* method_request,
    az_iot_status status,
    az_span response)
{
  int rc;

  // Get the Methods Response topic to publish the method response.
  char methods_response_topic_Buffer[128];
  rc = az_iot_hub_client_methods_response_get_publish_topic(
      &client, //&hub_client,
      method_request->request_id,
      (uint16_t)status,
      methods_response_topic_Buffer,
      sizeof(methods_response_topic_Buffere),
      NULL);
  if (az_result_failed(rc))
  {
    Serial.print(
    //IOT_SAMPLE_LOG_ERROR(
        //"Failed to get the Methods Response topic: az_result return code 0x%08x.", rc);
                "Failed to get the Methods Response topic: az_result return code 0x%08x.");
     Serial.println(rc);
    exit(rc);
  }

  // Publish the method response.
  mqtt_client.publish_P()
  rc = MQTTClient_publish(
      mqtt_client,
      methods_response_topic_Buffer,
      az_span_size(response),
      az_span_ptr(response),
      IOT_SAMPLE_MQTT_PUBLISH_QOS,
      0,
      NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    //IOT_SAMPLE_LOG_ERROR("Failed to publish the Methods response: MQTTClient return code %d.", rc);
    exit(rc);
  }
  //IOT_SAMPLE_LOG_SUCCESS("Client published the Methods response.");
  //IOT_SAMPLE_LOG("Status: %u", (uint16_t)status);
  //IOT_SAMPLE_LOG_AZ_SPAN("Payload:", response);
}*/

bool isNumeric(const char* s){
    while(*s){
        if(*s < '0' || *s > '9')
            return false;
        ++s;
    }
    return true;
}


//Perform the method with optional payload that is interpretted as an integer
bool DoMethod(char * method, char * payload)
{
  Serial.print("Method: ");
  Serial.print(method);
  int value = -1;
  if(payload != NULL)
  {
    // Actually should interpret as Json (2Do), but for simplicity just as a number fpr now. ???
    if(strlen(payload) != 0)
    {
      Serial.print(" Payload: ");
      Serial.print(payload);
      if(isNumeric(payload))
      {        
        value =  atoi(payload);
        Serial.print(" which is the number: ");
        Serial.print(value);
      }
      else
      {
        Serial.print(" which isn't a number.");
      }      
    }   
  }
  Serial.println(); 
  if (strncmp(method,"start",4)==0)
  {
    if(!IsRunning)
    {
      if(TelemetryFrequencyMilliseconds != 0)
      {
        next_telemetry_send_time_ms = millis();
        IsRunning = true;
        Serial.println("Telemtry was started.");
      }
      else
      {
        Serial.println("Telemetry is still stopped as period set to 0");
        return false;
      }
    }
  }
  else  if (strncmp(method,"stop",4)==0)
  {
    if(IsRunning)
    {
      IsRunning = false;
      Serial.println("Telemtry was stopped.");
    }
  }
  else if (strncmp(method,"frequency",4)==0)
  {
    if(value<0)
    {
      Serial.println(("Invalid Telemetry Period value. Should be no. seconds."));
      return false;
    }
    else
    {
      TelemetryFrequencyMilliseconds = value*1000;
      if(value==0)
      {
        IsRunning=false;
        Serial.println("Telemtry is stopped.");
      }
      else
      {
        Serial.print("Telemetry Period is now: ");
        Serial.print(value);
        Serial.println(" sec.");
        IsRunning= true;
        Serial.println("Telemtry is running.");
      }
    }
  }
  else if (strncmp(method,"toggle",4)==0)
  {
    LEDIsOn = ! LEDIsOn;
    if(LEDIsOn)
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
    }
    Serial.print("LED Toggled.");
  }
  else
  {
    Serial.print("Unrecognised Method: ");
    Serial.println(method);
    return false;
  }
  return true;
}

// Get char digits into an integer from the source as an az_span until char is not a digit
// Digits are HEX as per _az_NUMBER_OF_DECIMAL_VALUES
// From az_span_atou32() az_span.h https://azuresdkdocs.blob.core.windows.net/$web/c/az_iot/1.0.0/az__span_8h_source.html
AZ_NODISCARD az_result az_span_relaxed_atou32(az_span source, uint32_t* out_number)
{
  //_az_PRECONDITION_VALID_SPAN(source, 1, false);
  //_az_PRECONDITION_NOT_NULL(out_number);

  int32_t const span_size = az_span_size(source);

  if (span_size < 1)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // If the first character is not a digit or an optional + sign, return error.
  int32_t starting_index = 0;
  uint8_t* source_ptr = az_span_ptr(source);
  uint8_t next_byte = source_ptr[0];

  if (!isdigit(next_byte))
  {
    // There must be another byte after a sign.
    // The loop below checks that it must be a digit.
    if (next_byte != '+' || span_size < 2)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    starting_index++;
  }

  uint32_t value = 0;

  for (int32_t i = starting_index; i < span_size; ++i)
  {
    next_byte = source_ptr[i];
    if (!isdigit(next_byte)) 
    {
      if (i != starting_index)
         break;
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    uint32_t const d = (uint32_t)next_byte - '0';

    // Check whether the next digit will cause an integer overflow.
    // Before actually doing the math below, this is checking whether value * 10 + d > UINT32_MAX.
    /*if ((UINT32_MAX - d) / _az_NUMBER_OF_DECIMAL_VALUES < value)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }*/

    value = value * _az_NUMBER_OF_DECIMAL_VALUES + d;
  }

  *out_number = value;
  return AZ_OK;
}

DynamicJsonDocument messageResponseDoc(64);
char * originalMessageId;

void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  //messageResponseDoc.Clear();
  Serial.println(1234);
  char _topic[128];
  char _payload[128];
  char requestName[32];

  int topicLen = 0;
  while (topic[topicLen] !=0)
  {
    topicLen++;
  }

  // Get a 'porper' string from topic
  memset(_topic, '\0', sizeof(_topic));
  strncpy(_topic, topic, topicLen);
  Serial.println("============");
  Serial.println(_topic);
  Serial.println("============");

  //Get a 'proper' string from the payload
  memset(_payload, '\0', sizeof(_payload));
  strncpy(_payload, (char *)payload, length);
  Serial.println(length);

  if(strncmp(_topic,"$iothub/methods/",strlen("$iothub/methods/"))==0)
  {
    // Is a Direct Method

    //Get the Method Request
    az_iot_hub_client_method_request  request;
    az_span az_topic = az_span_create_from_str(_topic);
    az_result  res = az_iot_hub_client_methods_parse_received_topic(&client,az_topic, &request);

    if(res== AZ_OK )
    {
      memset(requestName, '\0', sizeof(requestName));
      az_span_to_str(requestName, sizeof(requestName), request.name);
      Serial.print("Received Method: [");
      Serial.print(requestName);

      Serial.print("] ");

      //Get request id
      uint32_t id;
      res = az_span_relaxed_atou32(request.request_id,&id);
      if(res== AZ_OK )
      {
        Serial.print("request_id: ");
        Serial.print(id);
      }
      else
      {
        Serial.println(res);
      }
         
      Serial.print(" ");
      Serial.print("Payload: ");
      Serial.println(_payload);

      //Call method
      bool methodResult = DoMethod(requestName,_payload);
      char * resp = get_Method_Response(request.request_id,requestName,_payload ,id, 200);
      bool responseResponse = mqtt_client.publish((char *)methodResponseBuffer, resp, false);
      if(methodResponseBuffer != NULL)
        free(methodResponseBuffer);
      if ( responseResponse)
      {
          Serial.println("A_OK: CD Method Response to Cloud");
      }
      else
      {
          Serial.println("N_OK: CD Method Response to Cloud");
      }
      //Need to send this result back ??
      /*
      {
          "status" : 201,
          "payload" : {...}
      }
      */
    }
  }
  else
  {
    // Is a Message
    Serial.print("Received  Message: [");
    Serial.print(_topic);
    Serial.print("] ");

    Serial.print("Payload: ");
    Serial.println(_payload);

    Ack_Mode == none;
    char * ackk = strstr(topic, "ack=");
    //Serial.print(">");
    //Serial.print(ackk);
    //Serial.println("<");
    ackk += strlen("ack=");
    //Serial.print(">");
    //Serial.print(ackk);
    //Serial.println("<");
    for (int i=0; i<4; i++)
    {
      if (strncmp( CD_Message_Ack[i],ackk,strlen(CD_Message_Ack[i]))==0)
      {
        Ack_Mode= (CD_MESSAGE_ACKS)i;
        Serial.print("Ack: ");
        Serial.println( CD_Message_Ack[(int)Ack_Mode]);
        break;
      }
    }
    if(Ack_Mode != none)
    {
      if (
        (Ack_Mode==full) || 
        ((Ack_Mode==postive) && (MsgOutcome==success)) || 
        ((Ack_Mode==negative) && (MsgOutcome != success))
      )
      {
        // Parse c2d message.
        az_iot_hub_client_c2d_request c2d_request;
        int topic_len = strlen(_topic);
        //Serial.print(">");
        //Serial.println(topic);
        char * messageId = strstr(topic, "&messageId=");
        //Serial.print(">");
        //Serial.println(messageId);
        messageId += strlen("&messageId=");
        //Serial.print(">");
        //Serial.println(messageId);

 
        
        if(originalMessageId!=NULL)
          free(originalMessageId);
        originalMessageId = (char *)  malloc(64);
        memset(originalMessageId, '\0', 64);
        int i=0;
        while (  (i < strlen(messageId)) && (i<63))
        {
          if(!isalnum (messageId[i]))
          {
            if (messageId[i] !='-')
              break;
          }
          originalMessageId[i] = messageId[i];
          i++;
        }
        //strncpy(originalMessageId,messageId,3);
        Serial.print("originalMessageId: ");
        Serial.println(originalMessageId);
        
      char jsonResponseStr[100];
      
        /* Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-c2d
        *  https://learn.microsoft.com/en-us/dotnet/api/microsoft.azure.devices.feedbackrecord.devicegenerationid?view=azure-dotnet
          {
            "originalMessageId": "0987654321",
            "enqueuedTimeUtc" : "2015-07-28T16:24:48.789Z",
            "statusCode" : "Success",
            "description" : "Success",
            "deviceId" : "123",
            "deviceGenerationId" : "abcdefghijklmnopqrstuvwxyz 
          }
        */

        //SEnding back the response for a Message doesn't seem to work
        if (strlen(originalMessageId)!= 0)
          messageResponseDoc["originalMessageId"] = originalMessageId;  //Autogenerated ??
        //messageResponseDoc["enqueuedTimeUtc"] = getCurrentLocalTimeString(); //Autogenerated
        messageResponseDoc["StatusCode"] = CD_Message_Outcome[(int)MsgOutcome];
        messageResponseDoc["Description"] = CD_Message_Outcome[(int)MsgOutcome];
        //messageResponseDoc["deviceId"] = IOT_CONFIG_DEVICE_ID;               // Autogenerated
        //messageResponseDoc["deviceGenerationId"] = IOT_CONFIG_DEVICE_KEY;    //Autogenerated
        serializeJson(messageResponseDoc, jsonResponseStr);
        Serial.println(jsonResponseStr);
        az_span temp_span = az_span_create_from_str(jsonResponseStr);
        az_span_to_str((char*)telemetry_payload, sizeof(telemetry_payload), temp_span);
        if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
              &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
        {
          Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
          return ;
        } 

        //Nb: By default, Acknoweledgement is not required for CD Messages.
        if(mqtt_client.publish(telemetry_topic,(char*)telemetry_payload, false))
        {     
            Serial.println("A_OK: CD MESSAGE Response to Cloud");
        }
        else
        {
            Serial.println("N_OK: CD MESSAGE Response to Cloud");
        };
      }
    }
  }
}

  DynamicJsonDocument doc(1024);
  char jsonStr[64];

  char* generateTelemetryPayload(uint32_t telemetry_send_count, int value)
  {   
      doc["value"] = value;
      doc["msgCount"] = telemetry_send_count;
      serializeJson(doc, jsonStr);
      az_span temp_span = az_span_create_from_str(jsonStr);
      az_span_to_str((char*)telemetry_payload, sizeof(telemetry_payload), temp_span);

      return (char*)telemetry_payload;
  }

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




