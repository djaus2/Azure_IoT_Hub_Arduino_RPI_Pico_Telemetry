
#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


#include "az_local_msglevels.h"
#include "az_local.h"
#include <string.h>

struct Properties Dev_Properties;
int NumTabs = 0;

char PropsJson[512] = "";

bool GotTwinDoc;

//Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-c2d
enum CD_MESSAGE_ACKS { none, full, postive, negative } Ack_Mode = ACK_MODE;
const char* CD_Message_Ack[] = { "none", "full", "postive", "negative" };
//Ref: https://learn.microsoft.com/en-us/dotnet/api/microsoft.azure.devices.feedbackstatuscode?view=azure-dotnet
enum CD_MESSAGE_OUTCOMES { success, expired, deliveryCountExceeded, rejected, purged } MsgOutcome = CD_MESSAGE_OUTCOME;
const char* CD_Message_Outcome[] = { "Success", "Expired", "Delivery Count Exceeded", "Rejected", "Purged" };


//bool IsRunning;
//bool LEDIsOn;
//unsigned long TelemetryFrequencyMilliseconds;
char telemetry_topic[128];
uint8_t telemetry_payload[1024]; 
az_iot_message_properties properties;



DynamicJsonDocument messageResponseDoc(1024);
DynamicJsonDocument messageReceivedDoc(1024);
char * originalMessageId;


void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  //messageResponseDoc.Clear();
  char _topic[512];
  char _payload[512];

  char requestName[32];
  PRINT_BEGIN("Got IoT Hub Doc-Message-Method-Response");
  {
      // Get a 'proper' string from topic
      int topicLen = 0;

      // Get topic without parameters
      int topicLen2 = 0;

      if (topic[0] == '$')
      {
          topicLen++;
      }
      while (topic[topicLen] != 0)
      {
          topicLen++;
      }

      memset(_topic, '\0', sizeof(_topic));
      strncpy(_topic, topic, topicLen);

      topicLen = 0;
      if (topic[0] == '$')
      {
          topicLen++;
      }
      while (_topic[topicLen] != 0)
      {
          //Search for first param: After last / or starts with $).
          if (_topic[topicLen] == '/')
          {
              topicLen2 = topicLen;
          }
          else if (_topic[topicLen] == '?')
          {
              topicLen2 = topicLen - 1;
              break;
          }
          topicLen++;
      }

      char shortTopic[128];
      memset(shortTopic, '\0', sizeof(shortTopic));
      strncpy(shortTopic, _topic, topicLen2);;

      //SERIAL_PRINT("  Topic: ");
      //SERIAL_PRINTLN((_topic);
      SERIALPRINT("Got Topic: ");
      SERIAL_PRINTLN(shortTopic);

      //Get a 'proper' string from the payload
      memset(_payload, '\0', sizeof(_payload));
      if (length != 0)
      {
          strncpy(_payload, (char*)payload, length);
          SERIALPRINTLN("  Got Payload");
      }
      else
      {
          SERIALPRINTLN("No Payload");
      }

      if (strncmp(_topic, "$iothub/twin/", strlen("$iothub/twin/")) == 0)
      {
          PRINT_BEGIN_SUB_1("Twin: ")
          {
              int responseType;
              char requestId[20];
              char tmp[512];
              strcpy(tmp, _payload);
              deserializeJson(messageReceivedDoc, _payload);
              strcpy(_payload, tmp);
              az_span const topic_span = az_span_create((uint8_t*)topic, 1 + strlen(_topic));
              az_span const message_span = az_span_create((uint8_t*)_payload, length);
              // Parse message and retrieve twin_response info.
              az_iot_hub_client_twin_response out_twin_response;

              az_result rc
                  = az_iot_hub_client_twin_parse_received_topic(&client, topic_span, &out_twin_response);
              if (az_result_failed(rc))
              {
                  SERIAL_PRINT("ERROR - Message from unknown topic: az_result return code: ");
                  SERIAL_PRINTLN(rc);
                  SERIAL_PRINT("Topic: ");
                  SERIAL_PRINTLN(_topic);
                  return;
              }
              else
              {
                  PRINT_BEGIN_SUB_2(" Client received a valid TWIN topic response.")
                  {
                      SERIALPRINT("  Topic:");
                      SERIAL_PRINTLN(topic); //topic_span);
                      SERIALPRINT("  Status: ");
                      int status = (int)out_twin_response.status;
                      SERIAL_PRINT(status);
                      SERIAL_PRINTLN(" 20X=OK");
                      SERIALPRINT("  ResponseType: ");
                      responseType = (int32_t)out_twin_response.response_type;
                      SERIAL_PRINTLN(responseType);
                      SERIALPRINTLN(" 1=Get,2=Desired,3=Reported,4-Error.");
                      if (responseType != 2)
                      {
                          SERIALPRINT("  RequestId: ");
                          (void)az_span_to_str(requestId, sizeof(requestId), out_twin_response.request_id);
                          SERIAL_PRINTLN(requestId);
                      }
                      if (status != 204)
                      {
                          /*SERIALPRINTLN("  Payload:");
                          SERIALPRINT("   ");
                          SERIALPRINTLN(_payload);*/
                      }
                      else
                      {
                          SERIALPRINTLN("  No Payload");
                      }

                      if (strncmp(_topic, "$iothub/twin/res/", strlen("$iothub/twin/res/")) == 0)
                      {
                          PRINT_BEGIN_SUB_3("Twin/res/... ")
                          {
                              if (responseType == 1)
                              {
                                  PRINT_BEGIN_SUB_4("   IoT Hub Twin Document GET Response: ")
                                  {
                                      SERIALPRINT("    Status: ");
                                      SERIAL_PRINT((int)out_twin_response.status);
                                      SERIAL_PRINTLN(" 200=OK, 204=OK and no return payload");
                                      SetProperties(_payload);
                                      GotTwinDoc = true;
                                  }
                                  PRINT_END_SUB_4
                              }
                              else if (responseType == 3)
                              {
                                  //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                                  PRINT_BEGIN_SUB_4("  IoT Hub Twin Property Update Response: ")
                                  {
                                      SERIALPRINT("   Status: ");
                                      SERIAL_PRINT((int)out_twin_response.status);
                                      SERIAL_PRINTLN(" 200=OK, 204=OK and no return payload");
                                      SERIALPRINT("   Version: ");
                                      uint32_t vern;
                                      az_result az = az_span_atou32(out_twin_response.version, &vern);
                                      SERIALPRINTLN(vern);
                                      SERIALPRINTLN(_payload);
                                  }
                                  PRINT_END_SUB_4
                              }
                              else if (responseType == 2)
                              {
                                  //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                                  PRINT_BEGIN_SUB_4("  IoT Hub Twin Property Desired: ")
                                  {
                                      SERIALPRINT("   Status: ");
                                      SERIAL_PRINT((int)out_twin_response.status);
                                      SERIAL_PRINTLN(" 200=OK, 204=OK and no return payload");
                                      SERIALPRINT("Version: ");
                                      uint32_t vern;
                                      az_result az = az_span_atou32(out_twin_response.version, &vern);
                                      SERIAL_PRINTLN(vern);
                                      SERIAL_PRINTLN(_payload);
                                  }
                                  PRINT_END_SUB_4
                              }
                              else if (responseType == 4)
                              {
                                  //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                                  PRINT_BEGIN_SUB_4(" IoT Hub Twin Property Error: ")
                                  {
                                      SERIALPRINT("Status: ");
                                      SERIAL_PRINT((int)out_twin_response.status);
                                      SERIAL_PRINTLN(" 200=OK, 204=OK and no return payload");
                                      SERIALPRINT("Version: ");
                                      uint32_t vern;
                                      az_result az = az_span_atou32(out_twin_response.version, &vern);
                                      SERIAL_PRINTLN(vern);
                                      SERIAL_PRINTLN(_payload);
                                  }
                                  PRINT_END_SUB_4
                              }
                              else
                              {
                                  SERIALPRINTLN("Unknown Twin Res");
                              }
                          }
                          PRINT_END_SUB_3
                      }
                      else  if (strncmp(_topic, "$iothub/twin/PATCH/", strlen("$iothub/twin/PATCH/")) == 0)
                      {
                          PRINT_BEGIN_SUB_3(" IoT Hub Document PATCH: ")
                          {
                              SERIALPRINTLN("Patch Document: ");
                              DynamicJsonDocument patchDoc(512);
                              deserializeJson(patchDoc, _payload);
                              serializeJsonPretty(patchDoc, Serial);
                              SERIALPRINTLN();
                              DynamicJsonDocument Props(512);
                              PRINT_BEGIN_SUB_4("Loading Current PropsJson on the device : ");
                              {
                                  if (strlen(PropsJson) == 0)
                                  {
                                      strcpy(PropsJson, "{}");
                                  }
                                  if ((strlen(PropsJson) != 0) && (strcmp(PropsJson, "{}") != 0))
                                  {
                                      deserializeJson(Props, PropsJson);

                                      JsonObject rootz = Props.as<JsonObject>();

                                      for (JsonPair kv : rootz) {
                                          SERIALPRINT(kv.key().c_str());
                                          SERIAL_PRINT(": ");
                                          SERIAL_PRINTLN(kv.value().as<String>());
                                      }
                                  }
                                  else
                                  {
                                      SERIALPRINTLN("Empty Properties on Device.");
                                  }
                              }
                              PRINT_END_SUB_4
                              if (strncmp(_topic, "$iothub/twin/PATCH/properties", strlen("$iothub/twin/PATCH/properties")) == 0)
                              {
                                  PRINT_BEGIN_SUB_4(" PATCH/properties: ")
                                  {
                                      if (strncmp(_topic, "$iothub/twin/PATCH/properties/desired/", strlen("$iothub/twin/PATCH/properties/desired")) == 0)
                                      {
                                          PRINT_BEGIN_SUB_5(" PATCH/properties/desired: ");
                                          {
                                              JsonObject rootx = patchDoc.as<JsonObject>();
                                              for (JsonPair kv : rootx) {
                                                  SERIALPRINT(kv.key().c_str());
                                                  SERIAL_PRINT(": ");
                                                  //JsonVariant val = kv.value();
                                                  //char* valStr = val.as<String>();
                                                  SERIAL_PRINTLN(kv.value().as<String>());
                                              }
                                              String patch = rootx["patchId"].as<String>();
                                              if (rootx["patchId"].isNull())
                                              {
                                                  SERIALPRINTLN("Null");
                                                  for (JsonPair kv : rootx) {
                                                      if (strncmp(kv.key().c_str(), "patchId", strlen("patchId")) == 0)
                                                      {
                                                          //continue;
                                                      }
                                                      else if (strncmp(kv.key().c_str(), "components", strlen("components")) == 0)
                                                      {
                                                          continue;
                                                      }
                                                      else if (strncmp(kv.key().c_str(), "module", strlen("module")) == 0)
                                                      {
                                                          continue;
                                                      }
                                                      Props[kv.key().c_str()] = NULL;
                                                  }
                                              }
                                              else //if (strncmp(patch.c_str(),"Init",strlen("Init"))==0)
                                              {
                                                  SERIALPRINTLN("Init");
                                                  for (JsonPair kv : rootx)
                                                  {
                                                      if (strncmp(kv.key().c_str(), "patchId", strlen("patchId")) == 0)
                                                      {
                                                          //continue;= 
                                                      }
                                                      else if ((!kv.value().is<JsonObject>()) && (!kv.value().is<JsonArray>()))
                                                      {
                                                          if (kv.value().is<String>())
                                                          {
                                                              String valStr = kv.value().as<String>();
                                                              if (strncmp(valStr.c_str(), "false", strlen("false")) == 0)
                                                              {
                                                                  Props[kv.key().c_str()] = false;
                                                              }
                                                              else if (strncmp(valStr.c_str(), "true", strlen("true")) == 0)
                                                              {
                                                                  Props[kv.key().c_str()] = true;
                                                              }
                                                              else if (strncmp(kv.key().c_str(), "components", strlen("components")) == 0)
                                                              {
                                                                  //SERIALPRINTLN("components");
                                                                  //Props[kv.key().c_str()]= true; 
                                                                  continue;
                                                              }
                                                              else if (strncmp(kv.key().c_str(), "modules", strlen("modules")) == 0)
                                                              {
                                                                  //SERIALPRINTLN("modules");
                                                                  //Props[kv.key().c_str()]= true; 
                                                                  continue;
                                                              }
                                                              else
                                                              {
                                                                  Props[kv.key().c_str()] = valStr;
                                                              }
                                                          }
                                                          else if (kv.value().is<int>())
                                                          {
                                                              Props[kv.key().c_str()] = kv.value().as<int>();
                                                          }
                                                          else if (kv.value().is<bool>())
                                                          {
                                                              Props[kv.key().c_str()] = kv.value().as<bool>();
                                                          }
                                                          else if (kv.value().is<double>())
                                                          {
                                                              Props[kv.key().c_str()] = kv.value().as<double>();
                                                          }
                                                      }
                                                      else
                                                      {
                                                          if (kv.value().is<JsonObject>())
                                                          {
                                                              SERIALPRINTLN("Object");
                                                          }
                                                          if (kv.value().is<JsonArray>())
                                                          {
                                                              SERIALPRINTLN("Array");
                                                          }
                                                      }
                                                  }
                                                  JsonObject rooty = Props.as<JsonObject>();
                                                  SERIALPRINTLN("....");
                                                  for (JsonPair kv : rooty) {
                                                      SERIALPRINT(kv.key().c_str());
                                                      SERIAL_PRINT(": ");
                                                      SERIAL_PRINTLN(kv.value().as<String>());
                                                  }
                                                  SERIALPRINTLN("....");
                                                  char numm[128];
                                                  serializeJson(rooty, numm);
                                                  if (strncmp(numm, "{\"\":", strlen("{\"\":")) == 0)
                                                  {
                                                      SERIALPRINTLN("Issue");
                                                      char temp[128];
                                                      char* tmp = numm + 0 + strlen("{\"\":");
                                                      strcpy(temp, "{\"fanOn\":");
                                                      strncat(temp, tmp, strlen(tmp));
                                                      memset(numm, '\0', sizeof(numm));
                                                      strcpy(numm, temp); //, strlen(temp));
                                                  }
                                                  strncpy(PropsJson, numm, strlen(numm));
                                                  SERIALPRINTLN(" - Saved Patched PropsJson on device.");
                                              }
 /*                                             }
                                              else
                                              {
                                                  SERIALPRINT("?");
                                              }*/
                                          }
                                          PRINT_END_SUB_5
                                      }
                                      else
                                      {
                                          SERIALPRINT("??");
                                      }
                                  }
                                  PRINT_END_SUB_4
                              }
                          }
                          PRINT_END_SUB_3
                      }
                  }
                  PRINT_END_SUB_2
              }
          }
          PRINT_END_SUB_1
      }
      else if (strncmp(_topic, "$iothub/methods/", strlen("$iothub/methods/")) == 0)
      {
        // Is a Direct Method
        PRINT_BEGIN_SUB_1(" IoT Hub Direct Method: ")
        {
            //Get the Method Request
            az_iot_hub_client_method_request  request;
            az_span az_topic = az_span_create_from_str(_topic);
            az_result  res = az_iot_hub_client_methods_parse_received_topic(&client, az_topic, &request);

            if (res == AZ_OK)
            {
                int param = -1;
                memset(requestName, '\0', sizeof(requestName));
                az_span_to_str(requestName, sizeof(requestName), request.name);
                SERIALPRINT("Received Method: [");
                SERIAL_PRINT(requestName);

                SERIAL_PRINTLN("]");

                //Get request id
                uint32_t id;
                res = az_span_relaxed_atou32(request.request_id, &id);
                if (res == AZ_OK)
                {
                    SERIALPRINT("request_id: ");
                    SERIAL_PRINTLN(id);
                }
                else
                {
                    SERIALPRINT("az_span Error: ")
                        SERIAL_PRINTLN(res);
                }

                if (strlen(_payload) == 0)
                {
                    SERIALPRINTLN("No payload");
                }
                else if (strncmp(_payload,"\"\"",2)==0)
                {
                    SERIALPRINTLN("No payload");
                }
                else
                {
                    SERIALPRINT("Payload: ");
                    SERIAL_PRINTLN(_payload);
                    param = atoi(_payload);
                }

                //Call method
                char tempStr[20];
                strcpy(tempStr, requestName);
                char tempStr2[20];
                strcpy(tempStr2, _payload);

                bool methodResult = DoMethod(tempStr, tempStr2);

                strcpy(tempStr, requestName);
                strcpy(tempStr2, _payload);


                
                char * resp = get_Method_Response(request.request_id, requestName, _payload, id, 200);
               
                bool responseResponse = mqtt_client.publish((char*)methodResponseBuffer, resp, false);
                if (methodResponseBuffer != NULL)
                    free(methodResponseBuffer);
                if (responseResponse)
                {
                    SERIALPRINTLN("A_OK: CD Method Response to Cloud");
                }
                else
                {
                    SERIALPRINTLN("N_OK: CD Method Response to Cloud");
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
        PRINT_END_SUB_1
      }
      else
      {
        // Is a Message
        PRINT_BEGIN_SUB_1("Received  Hub Message:");
        {
            SERIALPRINT("Topic: [");
            SERIAL_PRINT(_topic);
            SERIAL_PRINTLN("] ");
            if (strlen(_payload) == 0)
            {
                SERIALPRINTLN("No payload with Message");
            }
            if (strcmp("\"\"", _payload)==0)
            {
                SERIALPRINTLN("Empty payload with Message");
            }
            if (strcmp("{}", _payload) == 0)
            {
                SERIALPRINTLN("Empty Hson payload with Message");
            }
            else
            {
                SERIALPRINT("Payload: ");
                SERIAL_PRINTLN(_payload);
            }

            // Azure IOT Explorer sends an Ack property if optionally included  
            // VS Code no option to do so.
            // Sample app in this repository doesn't explicitly include it although the received rseponse there has it.
            Ack_Mode == none;
            char* ackk = strstr(topic, "ack=");
            ackk += strlen("ack=");
            for (int i = 0; i < 4; i++)
            {
                if (strncmp(CD_Message_Ack[i], ackk, strlen(CD_Message_Ack[i])) == 0)
                {
                    Ack_Mode = (CD_MESSAGE_ACKS)i;
                    SERIALPRINT("Ack: ");
                    SERIAL_PRINTLN(CD_Message_Ack[(int)Ack_Mode]);
                    break;
                }
            }
            if (Ack_Mode != none)
            {
                if (
                    (Ack_Mode == full) ||
                    ((Ack_Mode == postive) && (MsgOutcome == success)) ||
                    ((Ack_Mode == negative) && (MsgOutcome != success))
                    )
                {
                    // Parse c2d message.
                    // Yjis is a bit heuristic ...
                    // Look for &messageId=
                    // Or .mid="  in Topic
                    char MessageId[] = "&messageId=";
                    char MidId[] = ".mid=";
                    az_iot_hub_client_c2d_request c2d_request;
                    if (originalMessageId != NULL)
                        free(originalMessageId);
                    int buffSize = 64;
                    originalMessageId = (char*)malloc(buffSize);
                    memset(originalMessageId, '\0', buffSize);
                    int topic_len = strlen(_topic);

                    // Looking for &messageId-
                    char* messageId = strstr(topic, MessageId);

                    if (messageId == NULL)
                    {
                        // Then check for .mid=
                        // Example:
                        // devices/Pico10Dev/messages/devicebound/%24.mid=f2ceb710-e3ae-4a22-a3b2-607774d701b7&%...
                        // Looking for .mid=f2ceb710-e3ae-4a22-a3b2-607774d701b7& 
                        // ie terminated by &

                        messageId = strstr(topic, MidId);
                        messageId += strlen(MidId);
                        int i = 0;
                        while ((i < strlen(messageId)) && (i < (buffSize-1)))
                        {
                            // Accept alpjanumeric or - ... a Guid
                            if (messageId[i] == '&')
                                break;
                            else if (!isalnum(messageId[i]))
                            {
                                if (messageId[i] != '-')
                                    break;
                            }
                            originalMessageId[i] = messageId[i];
                            i++;
                        }
                    }
                    else
                    {
                        // devices/Pico10Dev/messages/devicebound/%24.to=%2Fdevices%2FPico10Dev%2Fmessages%2FdeviceBound&%24.ct=application%2Fjson&%24.ce=utf-8&messageId=823b1680-cebc-4752-b54f-bf57c7293d7e
                        // 8&messageId=823b1680-cebc-4752-b54f-bf57c7293d7e on end of Topic ...
                        messageId += strlen(MessageId);
                        // Azure IOT Explorer sends messageId if optionally included
                        // VS Code has no option to do
                        // Sample app in this repository doesn't explicitly include it although the received rseponse there has it.
                        int i = 0;
                        while ((i < strlen(messageId)) && (i < 63))
                        {
                            if (!isalnum(messageId[i]))
                            {
                                if (messageId[i] != '-')
                                    break;
                            }
                            originalMessageId[i] = messageId[i];
                            i++;
                        }
                    }
                    //strncpy(originalMessageId,messageId,3128);
                    SERIALPRINT("originalMessageId: ");
                    SERIAL_PRINTLN(originalMessageId);

                    /*
                    *  Construct feedback msg here and send to
                    * devices/<DeviceId>/Pico137Dev1/messages/servicebound/feedback
                    * Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-c2d
                    * ????
                    */

                }
            }
        }
        PRINT_END_SUB_1
      }
  }
  PRINT_END("END Callback")
}











