// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * This is an Arduino-based Azure IoT Hub sample for RPI Pico with Arduino installed. Based upon the ESPRESSIF ESP8266 board version.
 * It uses our Azure Embedded SDK for C to help interact with Azure IoT.
 * For reference, please visit https://github.com/azure/azure-sdk-for-c.
 *
 * To connect and work with Azure IoT Hub you need an MQTT client, connecting, subscribing
 * and publishing to specific topics to use the messaging features of the hub.
 * Our azure-sdk-for-c is an MQTT client support library, helping to compose and parse the
 * MQTT topic names and messages exchanged with the Azure IoT Hub.
 *
 * This sample performs the following tasks:
 * - Synchronize the device clock with a NTP server;
 * - Initialize our "az_iot_hub_client" (struct for data, part of our azure-sdk-for-c);
 * - Initialize the MQTT client (here we use Nick Oleary's PubSubClient, which also handle the tcp
 * connection and TLS);
 * - Connect the MQTT client (using server-certificate validation, SAS-tokens for client
 * authentication);
 * - Periodically send telemetry data to the Azure IoT Hub.
 *
 * To properly connect to your Azure IoT Hub, please fill the information in the `iot_configs.h`
 * file.
 */

// Sensors etc
#include <ArduinoJson.h>


// C99 libraries
#include <cstdlib>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Libraries for MQTT client, WiFi connection and SAS-token generation.

#include <WiFi.h>

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <bearssl/bearssl.h>
#include <bearssl/bearssl_hmac.h>
#include <libb64/cdecode.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// Additional sample headers
#include "iot_configs.h"

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'. (Modified this)
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;rpipico)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define ONE_HOUR_IN_SECS 3600
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_PACKET_SIZE 1024

// Translate iot_configs.h defines into variables used by the sample
static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const char* device_key = IOT_CONFIG_DEVICE_KEY;
static const int port = 8883;

// Memory allocated for the sample's variables and structures.
static WiFiClientSecure wifi_client;
static X509List cert((const char*)ca_pem);
static PubSubClient mqtt_client(wifi_client);
static az_iot_hub_client client;
static char sas_token[200];
static uint8_t signature[512];
static unsigned char encrypted_signature[32];
static char base64_decoded_device_key[32];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint8_t telemetry_payload[100];
static uint32_t telemetry_send_count = 0;

// Move to header if needed:
static bool isNumeric(const char* s);
static bool DoMethod(char * method, char * payload);
static char* get_Method_Response(az_span az_span_id, char * method, char * parameter, uint32_t id, uint16_t status); 
AZ_NODISCARD az_result az_span_relaxed_atou32(az_span source, uint32_t* out_number);

static bool IsRunning=false;
static bool LEDIsOn = false;
static unsigned long TelemetryFrequencyMilliseconds = TELEMETRY_FREQUENCY_MILLISECS;




char * methodResponseBuffer;
DynamicJsonDocument methodResponseDoc(64);
// Called by receivedCallback() if call is a CD Method call after the method is actioned
// Generate the payload to send back
static char* get_Method_Response(
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

#define _az_NUMBER_OF_DECIMAL_VALUES 16

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


// Auxiliary functions

static void connectToWiFi()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to WIFI SSID ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

static void initializeTime()
{
  Serial.print("Setting time using SNTP");

  configTime(-5 * 3600, 0, "pool.ntp.org","time.nist.gov"); 

  time_t now = time(NULL);
  while (now < 1510592825)
  {
    delay(500);
    Serial.print(".");
    now = time(NULL);
  }
  Serial.println("done!");
}

static char* getCurrentLocalTimeString()
{
  time_t now = time(NULL);
  return ctime(&now);
}

static void printCurrentTime()
{
  Serial.print("Current time: ");
  Serial.print(getCurrentLocalTimeString());
}

void receivedCallback(char* topic, byte* payload, unsigned int length)
{
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
      if ( responseResponse)
      {
          Serial.println("A_OK: CD Method Response to Cloud");
      }
      else
      {
          Serial.println("N_OK: CD Method Response to Cloud");
      }
      free(methodResponseBuffer);
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
  }
}

static void initializeClients()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  wifi_client.setTrustAnchors(&cert);
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
  {
    Serial.println("Failed initializing Azure IoT Hub client");
    return;
  }

  mqtt_client.setServer(host, port);
  mqtt_client.setCallback(receivedCallback);
//mqtt_client.
}

/*
 * @brief           Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */
static uint32_t getSecondsSinceEpoch() { return (uint32_t)time(NULL); }

static int generateSasToken(char* sas_token, size_t size)
{
  az_span signature_span = az_span_create((uint8_t*)signature, sizeofarray(signature));
  az_span out_signature_span;
  az_span encrypted_signature_span
      = az_span_create((uint8_t*)encrypted_signature, sizeofarray(encrypted_signature));

  uint32_t expiration = getSecondsSinceEpoch() + ONE_HOUR_IN_SECS;

  // Get signature
  if (az_result_failed(az_iot_hub_client_sas_get_signature(
          &client, expiration, signature_span, &out_signature_span)))
  {
    Serial.println("Failed getting SAS signature");
    return 1;
  }

  // Base64-decode device key
  int base64_decoded_device_key_length
      = base64_decode_chars(device_key, strlen(device_key), base64_decoded_device_key);

  if (base64_decoded_device_key_length == 0)
  {
    Serial.println("Failed base64 decoding device key");
    return 1;
  }

  // SHA-256 encrypt
  br_hmac_key_context kc;
  br_hmac_key_init(
      &kc, &br_sha256_vtable, base64_decoded_device_key, base64_decoded_device_key_length);

  br_hmac_context hmac_ctx;
  br_hmac_init(&hmac_ctx, &kc, 32);
  br_hmac_update(&hmac_ctx, az_span_ptr(out_signature_span), az_span_size(out_signature_span));
  br_hmac_out(&hmac_ctx, encrypted_signature);

  // Base64 encode encrypted signature
  String b64enc_hmacsha256_signature = base64::encode(encrypted_signature, br_hmac_size(&hmac_ctx));

  az_span b64enc_hmacsha256_signature_span = az_span_create(
      (uint8_t*)b64enc_hmacsha256_signature.c_str(), b64enc_hmacsha256_signature.length());

  // URl-encode base64 encoded encrypted signature
  if (az_result_failed(az_iot_hub_client_sas_get_password(
          &client,
          expiration,
          b64enc_hmacsha256_signature_span,
          AZ_SPAN_EMPTY,
          sas_token,
          size,
          NULL)))
  {
    Serial.println("Failed getting SAS token");
    return 1;
  }

  return 0;
}

static int connectToAzureIoTHub()
{
  size_t client_id_length;
  char mqtt_client_id[128];
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Serial.println("Failed getting client id");
    return 1;
  }

  mqtt_client_id[client_id_length] = '\0';

  char mqtt_username[128];
  // Get the MQTT user name used to connect to IoT Hub
  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    printf("Failed to get MQTT clientId, return code\n");
    return 1;
  }

  Serial.print("Client ID: ");
  Serial.println(mqtt_client_id);

  Serial.print("Username: ");
  Serial.println(mqtt_username);

  mqtt_client.setBufferSize(MQTT_PACKET_SIZE);

  while (!mqtt_client.connected())
  {
    time_t now = time(NULL);

    Serial.print("MQTT connecting ... ");

    if (mqtt_client.connect(mqtt_client_id, mqtt_username, sas_token))
    {
      Serial.println("connected.");
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(mqtt_client.state());
      Serial.println(". Trying again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);  

  return 0;
}

static void establishConnection()
{
  connectToWiFi();
  initializeTime();
  printCurrentTime();
  initializeClients();

  // The SAS token is valid for 1 hour by default in this sample.
  // After one hour the sample must be restarted, or the client won't be able
  // to connect/stay connected to the Azure IoT Hub.
  if (generateSasToken(sas_token, sizeofarray(sas_token)) != 0)
  {
    Serial.println("Failed generating MQTT password");
  }
  else
  {
    connectToAzureIoTHub();
  }

  digitalWrite(LED_BUILTIN, LOW);
}

#define PIN_ADC0   26

DynamicJsonDocument doc(1024);
char jsonStr[64];
char ret[64];

static char* getTelemetryPayload()
{
    int adcValue = analogRead(PIN_ADC0);                            //read ADC pin
    doc["msgCount"]   = telemetry_send_count ++;
    doc["value"]   = map(adcValue, 0, 1023, 0, 255);
    serializeJson(doc, jsonStr);
    az_span temp_span = az_span_create_from_str(jsonStr);
    az_span_to_str((char *)telemetry_payload, sizeof(telemetry_payload), temp_span);

  return (char*)telemetry_payload;
}

static void sendTelemetry()
{
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print(millis());
  
  Serial.print(" RPI Pico (Arduino) Sending telemetry . . . ");
 
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }
  char *   payload = getTelemetryPayload();
  Serial.println(payload);
  if (strlen(payload)!= 0)
  {
    mqtt_client.publish(telemetry_topic, payload, false);
    Serial.println("OK");
  }
  else
    Serial.println(" NOK");
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}

// Arduino setup and loop main functions.

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LEDIsOn);
  establishConnection();
}

void loop()
{
  if(IsRunning)
  {
  if (millis() > next_telemetry_send_time_ms)
  {
      // Check if connected, reconnect if needed.
      if (!mqtt_client.connected())
      {
        establishConnection();
      }

      sendTelemetry();
      next_telemetry_send_time_ms = millis() + TelemetryFrequencyMilliseconds;
    }
  }

  // MQTT loop must be called to process Device-to-Cloud and Cloud-to-Device.
  mqtt_client.loop();
  delay(500);
}



