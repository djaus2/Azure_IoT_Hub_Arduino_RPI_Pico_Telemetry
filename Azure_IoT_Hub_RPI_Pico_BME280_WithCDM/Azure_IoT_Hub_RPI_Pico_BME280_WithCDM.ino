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

#include "iot_configs.h"

// Sensors etc
/*

This code shows how to record data from the BME280 environmental sensor
using I2C interface. 


Connecting the BME280 Sensor:
Sensor              ->  Board
-----------------------------
Vin (Voltage In)    ->  3.3V
Gnd (Ground)        ->  Gnd
SDA (Serial Data)   ->  4
SCK (Serial Clock)  ->  5

 */

#include <BME280I2C.h>
#include <Wire.h>
BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   //BME280I2C::I2CAddr_0x76 
   BME280I2C::I2CAddr_0x77
);
//BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure �1, temperature �1, humidity �1, filter off,

//////////////////////////////////////////////////////////////////
BME280I2C bme(settings);


// C99 libraries
#include <cstdlib>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Libraries for MQTT client, WiFi connection and SAS-token generation.

#include <WiFi.h>


#include <WiFiClientSecure.h>
#include <base64.h>
#include <bearssl/bearssl.h>
#include <bearssl/bearssl_hmac.h>
#include <libb64/cdecode.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "src/az_local/az_local.h"
//void get_device_twin_document(void);

 bool DoSetHardware = false;
 bool isRestarting= false;


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

unsigned long next_telemetry_send_time_ms;
uint32_t telemetry_send_count;

bool MethodsSubscribed = false;
bool CDMessagesSubscribed = false;
bool TwinResponseSubscribed = false;
bool TwinPatchSubscribed = false;

static const int port = 8883;

// Memory allocated for the sample's variables and structures.
static WiFiClientSecure wifi_client;
static X509List cert((const char*)ca_pem);
PubSubClient mqtt_client(wifi_client);
az_iot_hub_client client;
static char sas_token[200];
static uint8_t signature[512];
static unsigned char encrypted_signature[32];
static char base64_decoded_device_key[32];


// Auxiliary functions

static void connectToWiFi()
{
  while(!Serial){}
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

 char* getCurrentLocalTimeString()
{
  time_t now = time(NULL);
  return ctime(&now);
}

static void printCurrentTime()
{
  Serial.print("Current time: ");
  Serial.print(getCurrentLocalTimeString());
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
  MethodsSubscribed = true;

  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
  CDMessagesSubscribed = true;

  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC );
  TwinResponseSubscribed = true;

  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);
  TwinPatchSubscribed = true;
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
  Dev_Properties.LEDIsOn = false;
}

DynamicJsonDocument doc(1024);
char jsonStr[128];
char ret[64];

static char telemetryNamesStr[] = TELEMETRY_NAMES;
double * telemetry;
char ** telemetryNames;
int numTelemetryNames=0;


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. 
       Also note number of token not including last empty string.
       */
    count++;
    numTelemetryNames = count-1;
 
    result = (char **) malloc(sizeof(char*) * count);
    telemetry = (double *) malloc(sizeof(double) * numTelemetryNames);
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


void getTelemetryNames()
{
   telemetryNames = str_split(telemetryNamesStr, ',');
  Serial.println();
  Serial.println("Telemetry:");
  Serial.println("============");
  /* int i=0;
   while (telemetryNames[i] != NULL)
   {
        Serial.print(">>");
        Serial.print(telemetryNames[i++]);
        Serial.println("<<");
   }

   i=0;
   while (strlen(telemetryNames[i]) != 0)
   {
        Serial.print(">>");
        Serial.print(telemetryNames[i++]);
        Serial.println("<<");
   }*/

   for (int i=0;i<numTelemetryNames;i++)
   {
        Serial.println(telemetryNames[i]);
   }
 /*
   Serial.println(telemetryNames[otemp]);
   Serial.println(telemetryNames[opres]);
   Serial.println(telemetryNames[ohum]);
  */
}



// Don't want too many decimal plces in the json string
double Round(float value, long n)
{
    long trunc  = 1;
    for (int i=0;i<n;i++)
    {
      trunc *=10;
    }    
    double dValue = value;
    long ret = round(dValue* trunc);
    return (((double) ret) / trunc);
}

double Round2Places(float value)
{
    if (value == NAN)
      return NAN;
    double ret = Round(value,2);
    return ret;
}


static bool ReadSensor()
{
  Stream* client = &Serial;
   float temp(NAN), hum(NAN), press(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(press, temp, hum, tempUnit, presUnit);

    telemetry[otemp] = Round2Places(temp);
    telemetry[opres] = Round2Places(press);   
    telemetry[ohum] = Round2Places(hum);

    for (int i=0;i<numTelemetryNames;i++)
    {
      if (i!=0)
      {
        client->print("\t");
      }
      client->print(telemetryNames[i]);
      client->print(": ");
      client->print(telemetry[i]);
      switch (i)
      {
        case otemp:
          client->print("°" + String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
          break;
        case ohum:
          client->print("% RH");
          break;
        case opres:
          client->println("Pa");
          break;
      }
    }
    Serial.println();
   return true;
}

static char* getTelemetryPayload()
{
  bool chk = ReadSensor();
  if (chk) {
    doc["msgCount"]   = telemetry_send_count ++;
    for (int i=0;i<numTelemetryNames;i++)
    {
      doc[telemetryNames[i]]   = telemetry[i];
    }
    serializeJson(doc, jsonStr);
    az_span temp_span = az_span_create_from_str(jsonStr);
    az_span_to_str((char *)telemetry_payload, sizeof(telemetry_payload), temp_span);
  }
  else
    telemetry_payload[0] = 0;
  return (char*)telemetry_payload;
}

static void sendTelemetry()
{
  if(isRestarting)
    return;
  digitalWrite(LED_BUILTIN, HIGH);
  Dev_Properties.LEDIsOn = true;

  Serial.print(millis());
  
  Serial.println(" RPI Pico (Arduino) Sending telemetry . . . ");


  char *   payload = getTelemetryPayload();
  
  // Add a property to the message  
  az_iot_message_properties * properties = GetProperties(telemetry[otemp]);
  
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, properties, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  Serial.println(telemetry_topic);
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
  Dev_Properties.LEDIsOn = false;
}
bool SentProp;
// Arduino setup and loop main functions.


void hwSetup()
{
  while (!Serial){}

  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
}


void setup()
{
  Serial.begin(115200);
  while(!Serial){}	
  getTelemetryNames();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  GotTwinDoc=false;

  MethodsSubscribed = false;
  CDMessagesSubscribed = false;
  TwinResponseSubscribed = false;
  TwinPatchSubscribed = false;

  while(!Serial)
  {}
  establishConnection();
  InitProperties();
  SaveProperties();
  LoadProperties();
  //PrintProperties();
  //PrintStructProperties();
  next_telemetry_send_time_ms = 0;
  telemetry_send_count = 0;
  SentProp = false;
  delay(1000);
  get_device_twin_document();
  isRestarting= false;
  
  hwSetup();
}

void loop()
{
  if (isRestarting)
  {
    delay(500);
    return;
  }
  if (GotTwinDoc)
  {
    if (!SentProp)
    {    
        ReportProperties();
        SentProp=true;
    }
  }
  if(DoSetHardware)
  {
    SetHardware();
  }
  if(Dev_Properties.IsRunning)
  {
      if (millis() > next_telemetry_send_time_ms)
      {
          // Check if connected, reconnect if needed.
          if (!mqtt_client.connected())
          {
            establishConnection();
          }

          sendTelemetry();

          next_telemetry_send_time_ms = millis() + Dev_Properties.TelemetryFrequencyMilliseconds;
      }
  }

  // MQTT loop must be called to process Device-to-Cloud and Cloud-to-Device.
  mqtt_client.loop();
  delay(500);
}

void SetHardware()
{
}


void Restart()
{
    
    isRestarting = true;
    Dev_Properties.IsRunning = false;
     delay(2000);
      mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);  
      MethodsSubscribed = false;

      mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
      CDMessagesSubscribed = false;

      mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC );
      TwinResponseSubscribed = false;

      mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);
      TwinPatchSubscribed = false;
    mqtt_client.disconnect();
    wifi_client.stop();
    WiFi.end();
    if (Serial)
        Serial.end();
    setup();
}



