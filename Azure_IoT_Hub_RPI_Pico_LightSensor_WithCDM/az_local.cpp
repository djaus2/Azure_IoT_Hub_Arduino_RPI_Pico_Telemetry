
#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local.h"

#include "iot_configs.h"
#include <string.h>

struct Properties Dev_Properties;


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
      int topicLen = 0;
      while (topic[topicLen] != 0)
      {
          topicLen++;
      }

      // Get a 'proper' string from topic
      memset(_topic, '\0', sizeof(_topic));
      strncpy(_topic, topic, topicLen);
      Serial.print("  Topic: ");
      Serial.println(_topic);

      //Get a 'proper' string from the payload

      memset(_payload, '\0', sizeof(_payload));
      if (length != 0)
      {
          strncpy(_payload, (char*)payload, length);
          Serial.println("  Got Payload");
          Serial.println(_payload);
      }

      if (strncmp(_topic, "$iothub/twin/", strlen("$iothub/twin/")) == 0)
      {
          PRINT_BEGIN_SUB("Twin: ")
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
                  Serial.print("ERROR - Message from unknown topic: az_result return code: ");
                  Serial.println(rc, HEX);
                  Serial.print("Topic: ");
                  Serial.println(_topic);
                  return;
              }
              else
              {
                  PRINT_BEGIN_SUB_SUB(" Client received a valid TWIN topic response.")
                  {
                      Serial.print("  Topic:");
                      Serial.println(topic); //topic_span);
                      Serial.print("  Status: ");
                      int status = (int)out_twin_response.status;
                      Serial.print(status);
                      Serial.println(" 20X=OK");
                      Serial.print("  ResponseType: ");
                      responseType = (int32_t)out_twin_response.response_type;
                      Serial.print(responseType);
                      Serial.println(" 1=Get,2=Desired,3=Reported,4-Error.");
                      if (responseType != 2)
                      {
                          Serial.print("  RequestId: ");
                          (void)az_span_to_str(requestId, sizeof(requestId), out_twin_response.request_id);
                          Serial.println(requestId);
                      }
                      if (status != 204)
                      {
                          /*Serial.println("  Payload:");
                          Serial.print("   ");
                          Serial.println(_payload);*/
                      }
                      else
                      {
                          Serial.println("  No Payload");
                      }

                      if (strncmp(_topic, "$iothub/twin/res/", strlen("$iothub/twin/res/")) == 0)
                      {
                          PRINT_BEGIN_SUB_SUB_SUB("Twin/res/... ");
                          if (responseType == 1)
                          {
                              PRINT_BEGIN_SUB_SUB_SUB_SUB("   IoT Hub Twin Document GET Response: ")
                              {
                                  Serial.print("    Status: ");
                                  Serial.print((int)out_twin_response.status);
                                  Serial.println(" 200=OK, 204=OK and no return payload");
                                  SetProperties(_payload);
                                  GotTwinDoc = true;
                              }
                              PRINT_END_SUB_SUB_SUB_SUB
                          }
                          else if (responseType == 3)
                          {
                              //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                              PRINT_BEGIN_SUB_SUB_SUB("  IoT Hub Twin Property Update Response: ")
                              {
                                  Serial.print("   Status: ");
                                  Serial.print((int)out_twin_response.status);
                                  Serial.println(" 200=OK, 204=OK and no return payload");
                                  Serial.print("   Version: ");
                                  uint32_t vern;
                                  az_result az = az_span_atou32(out_twin_response.version, &vern);
                                  Serial.println(vern);
                                  Serial.println(_payload);
                              }
                              PRINT_END_SUB_SUB_SUB_SUB
                          }
                          else if (responseType == 2)
                          {
                              //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                              PRINT_BEGIN_SUB_SUB_SUB_SUB("  IoT Hub Twin Property Desired: ")
                              {
                                  Serial.print("   Status: ");
                                  Serial.print((int)out_twin_response.status);
                                  Serial.println(" 200=OK, 204=OK and no return payload");
                                  Serial.print("Version: ");
                                  uint32_t vern;
                                  az_result az = az_span_atou32(out_twin_response.version, &vern);
                                  Serial.println(vern);
                                  Serial.println(_payload);
                              }
                              PRINT_END_SUB_SUB_SUB_SUB
                          }
                          else if (responseType == 4)
                          {
                              //Topic:$iothub/twin/res/204/?$rid=reported_prop&$version=59
                              PRINT_BEGIN_SUB_SUB_SUB_SUB(" IoT Hub Twin Property Error: ")
                              {
                                  Serial.print("Status: ");
                                  Serial.print((int)out_twin_response.status);
                                  Serial.println(" 200=OK, 204=OK and no return payload");
                                  Serial.print("Version: ");
                                  uint32_t vern;
                                  az_result az = az_span_atou32(out_twin_response.version, &vern);
                                  Serial.println(vern);
                                  Serial.println(_payload);
                              }
                              PRINT_END_SUB_SUB_SUB_SUB
                          }
                          else
                          {
                              Serial.println("Unknown Twin Res");
                          }
                          PRINT_END_SUB_SUB_SUB
                      }
                      else  if (strncmp(_topic, "$iothub/twin/PATCH/", strlen("$iothub/twin/PATCH/")) == 0)
                      {
                          PRINT_BEGIN_SUB_SUB(" IoT Hub Document PATCH: ")
                          {
                              Serial.println("Patch Document: ");
                              DynamicJsonDocument patchDoc(512);
                              deserializeJson(patchDoc, _payload);
                              serializeJsonPretty(patchDoc, Serial);
                              Serial.println();
                              Serial.println("-------------------");
                              DynamicJsonDocument Props(512);
                              if (strlen(PropsJson) == 0)
                              {
                                  strcpy(PropsJson, "{}");
                              }
                              Serial.println("Loading Current PropsJson on the device: ");
                              Serial.println("-------------------");
                              if ((strlen(PropsJson) != 0) && (strcmp(PropsJson,"{}") !=0))
                              {
                                  deserializeJson(Props, PropsJson);

                                  JsonObject rootz = Props.as<JsonObject>();

                                  for (JsonPair kv : rootz) {
                                      Serial.print(kv.key().c_str());
                                      Serial.print(": ");
                                      Serial.println(kv.value().as<String>());
                                  }
                              }
                              else
                              {
                                  Serial.println("Empty Properties on Device.");
                              }
                              if (strncmp(_topic, "$iothub/twin/PATCH/properties", strlen("$iothub/twin/PATCH/properties")) == 0)
                              {
                                  PRINT_BEGIN_SUB_SUB_SUB(" PATCH/properties: ")
                                  {
                                      if (strncmp(_topic, "$iothub/twin/PATCH/properties/desired/", strlen("$iothub/twin/PATCH/properties/desired")) == 0)
                                      {
                                          PRINT_BEGIN_SUB_SUB_SUB_SUB(" PATCH/properties/desired: ");
                                          {
                                              JsonObject rootx = patchDoc.as<JsonObject>();
                                              for (JsonPair kv : rootx) {
                                                  Serial.print(kv.key().c_str());
                                                  Serial.print(": ");
                                                  //JsonVariant val = kv.value();
                                                  //char* valStr = val.as<String>();
                                                  Serial.println(kv.value().as<String>());
                                              }
                                              String patch = rootx["patchId"].as<String>();
                                              if (rootx["patchId"].isNull())
                                              {
                                                  Serial.println("Null");
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
                                                  Serial.println("Init");
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
                                                                  //Serial.println("components");
                                                                  //Props[kv.key().c_str()]= true; 
                                                                  continue;
                                                              }
                                                              else if (strncmp(kv.key().c_str(), "modules", strlen("modules")) == 0)
                                                              {
                                                                  //Serial.println("modules");
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
                                                              Serial.println("Object");
                                                          }
                                                          if (kv.value().is<JsonArray>())
                                                          {
                                                              Serial.println("Array");
                                                          }
                                                      }
                                                  }
                                                  JsonObject rooty = Props.as<JsonObject>();
                                                  Serial.println("....");
                                                  for (JsonPair kv : rooty) {
                                                      Serial.print(kv.key().c_str());
                                                      Serial.print(": ");
                                                      Serial.println(kv.value().as<String>());
                                                  }
                                                  Serial.println("....");
                                                  char numm[128];
                                                  serializeJson(rooty, numm);
                                                  if (strncmp(numm, "{\"\":", strlen("{\"\":")) == 0)
                                                  {
                                                      Serial.println("Issue");
                                                      char temp[128];
                                                      char* tmp = numm + 0 + strlen("{\"\":");
                                                      strcpy(temp, "{\"fanOn\":");
                                                      strncat(temp, tmp, strlen(tmp));
                                                      memset(numm, '\0', sizeof(numm));
                                                      strcpy(numm, temp); //, strlen(temp));
                                                  }
                                                  strncpy(PropsJson, numm, strlen(numm));
                                                  Serial.println("-----");
                                                  Serial.print("Saved Patched PropsJson on device: ");
                                                  Serial.println("-----");

                                              }
 /*                                             }
                                              else
                                              {
                                                  Serial.print("?");
                                              }*/
                                          }
                                          PRINT_END_SUB_SUB_SUB_SUB
                                      }
                                      else
                                      {
                                          Serial.print("??");
                                      }
                                  }
                                  PRINT_END_SUB_SUB_SUB
                              }
                              Serial.println();
                              Serial.println(_payload);
                          }
                          PRINT_END_SUB_SUB
                      }
                  }
                  PRINT_END_SUB_SUB
              }
          }
          PRINT_END_SUB
      }
      else if (strncmp(_topic, "$iothub/methods/", strlen("$iothub/methods/")) == 0)
      {
        // Is a Direct Method
        PRINT_BEGIN_SUB(" IoT Hub Direct Method: ")
        {
            //Get the Method Request
            az_iot_hub_client_method_request  request;
            az_span az_topic = az_span_create_from_str(_topic);
            az_result  res = az_iot_hub_client_methods_parse_received_topic(&client, az_topic, &request);

            if (res == AZ_OK)
            {
                memset(requestName, '\0', sizeof(requestName));
                az_span_to_str(requestName, sizeof(requestName), request.name);
                Serial.print("Received Method: [");
                Serial.print(requestName);

                Serial.print("] ");

                //Get request id
                uint32_t id;
                res = az_span_relaxed_atou32(request.request_id, &id);
                if (res == AZ_OK)
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
                bool methodResult = DoMethod(requestName, _payload);
                char* resp = get_Method_Response(request.request_id, requestName, _payload, id, 200);
                bool responseResponse = mqtt_client.publish((char*)methodResponseBuffer, resp, false);
                if (methodResponseBuffer != NULL)
                    free(methodResponseBuffer);
                if (responseResponse)
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
        PRINT_END_SUB
      }
      else
      {
        // Is a Message
        PRINT_BEGIN_SUB("Received  Hub Message:");
        {
            Serial.print("Topic: [");
            Serial.print(_topic);
            Serial.print("] ");

            Serial.print("Payload: ");
            Serial.println(_payload);

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
                    Serial.print("Ack: ");
                    Serial.println(CD_Message_Ack[(int)Ack_Mode]);
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
                    az_iot_hub_client_c2d_request c2d_request;
                    int topic_len = strlen(_topic);
                    char* messageId = strstr(topic, "&messageId=");
                    messageId += strlen("&messageId=");

                    // Azure IOT Explorer sends messageId if optionally included
                    // VS Code has no option to do
                    // Sample app in this repository doesn't explicitly include it although the received rseponse there has it.
                    if (originalMessageId != NULL)
                        free(originalMessageId);
                    originalMessageId = (char*)malloc(64);
                    memset(originalMessageId, '\0', 64);
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
                    //strncpy(originalMessageId,messageId,3128);
                    Serial.print("originalMessageId: ");
                    Serial.println(originalMessageId);

                    /*
                    *  Construct feedback msg here and send to
                    * devices/<DeviceId>/Pico137Dev1/messages/servicebound/feedback
                    * Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-c2d
                    * ????
                    */

                }
            }
        }
        PRINT_END_SUB
      }
  }
  PRINT_END
}











