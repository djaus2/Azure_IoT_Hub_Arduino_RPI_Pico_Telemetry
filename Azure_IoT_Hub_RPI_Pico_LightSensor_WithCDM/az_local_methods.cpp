#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local.h"

#include "iot_configs.h"
#include <string.h>

char* methodResponseBuffer;
DynamicJsonDocument methodResponseDoc(64);


//Perform the method with optional payload that is interpretted as an integer
bool DoMethod(char* method, char* payload)
{
    Serial.print("Method: ");
    Serial.print(method);
    int value = -1;
    if (payload != NULL)
    {
        // Actually should interpret as Json (2Do), but for simplicity just as a number fpr now. ???
        if (strlen(payload) != 0)
        {
            Serial.print(" Payload: ");
            Serial.print(payload);
            if (isNumeric(payload))
            {
                value = atoi(payload);
                Serial.print(" which is the number: ");
                Serial.print(value);
            }
            else
            {
                if (strlen(payload) > 2)
                {
                    if (strncmp(payload, "method", 3) == 0)
                    {
                        value = 0;
                    }
                    else if (strncmp(payload, "message", 3) == 0)
                    {
                        value = 1;
                    }
                    else if (strncmp(payload, "msg", 3) == 0)
                    {
                        value = 1;
                    }
                    else
                    {
                        Serial.print(" which isn't a number.");
                    }
                }
                else
                {
                    Serial.print(" which isn't a number.");
                }
            }
        }
    }
    Serial.println();
    if (strncmp(method, "start", 4) == 0)
    {
        if (!Dev_Properties.IsRunning)
        {
            if (Dev_Properties.TelemetryFrequencyMilliseconds != 0)
            {
                next_telemetry_send_time_ms = millis();
                Dev_Properties.IsRunning = true;
                Serial.println("Telemtry was started.");
            }
            else
            {
                Serial.println("Telemetry is still stopped as period set to 0");
                return false;
            }
        }
    }
    else  if (strncmp(method, "stop", 4) == 0)
    {
        if (Dev_Properties.IsRunning)
        {
            Dev_Properties.IsRunning = false;
            Serial.println("Telemtry was stopped.");
        }
    }
    else if (strncmp(method, "frequency", 4) == 0)
    {
        if (value < 0)
        {
            Serial.println(("Invalid Telemetry Period value. Should be no. seconds."));
            return false;
        }
        else
        {
            Dev_Properties.TelemetryFrequencyMilliseconds = value * 1000;
            if (value == 0)
            {
                Dev_Properties.IsRunning = false;
                Serial.println("Telemtry is stopped.");
            }
            else
            {
                Serial.print("Telemetry Period is now: ");
                Serial.print(value);
                Serial.println(" sec.");
                Dev_Properties.IsRunning = true;
                Serial.println("Telemtry is running.");
            }
        }
    }
    else if (strncmp(method, "toggle", 4) == 0)
    {
        Dev_Properties.LEDIsOn = !Dev_Properties.LEDIsOn;
        if (Dev_Properties.LEDIsOn)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            Dev_Properties.LEDIsOn = true;
        }
        else
        {
            digitalWrite(LED_BUILTIN, LOW);
            Dev_Properties.LEDIsOn = false;
        }
        Serial.print("LED Toggled.");
    }
    else if (strncmp(method, "subscribe", 4) == 0)
    {
        if (value == 0)
        {
            if (Dev_Properties.MethodsSubscribed == false)
            {
                mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
                Dev_Properties.MethodsSubscribed = true;
                Serial.println("CD METHODS turned OMN.");
            }
        }
        else if (value == 1)
        {
            if (Dev_Properties.CDMessagesSubscribed == false)
            {
                mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
                Dev_Properties.CDMessagesSubscribed = true;
                Serial.println("CD MESSAGES turned ON.");
            }
        }
        else
        {
            Serial.println("Invalid Subscription to turn on.");
        }
    }
    else if (strncmp(method, "unsubcribe", 4) == 0)
    {
        if (value == 0)
        {
            if (Dev_Properties.MethodsSubscribed == true)
            {
                mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
                Dev_Properties.MethodsSubscribed = false;
                Serial.println("CD METHODS turned OFF.");
            }
        }
        else  if (value == 1)
        {
            if (Dev_Properties.MethodsSubscribed == true)
            {
                mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
                Dev_Properties.CDMessagesSubscribed = false;
                Serial.println("CD MESSAGES turned OFF.");
            }
        }
        else
        {
            Serial.println("Invalid Subscription to turn off.");
        }
    }
    else
    {
        Serial.print("Unrecognised Method: ");
        Serial.println(method);
        return false;
    }
    return true;
}

// Called by receivedCallback() if call is a CD Method call after the method is actioned
// Generate the payload to send back
char* get_Method_Response(
    az_span az_span_id,
    char* method,
    char* parameter,
    uint32_t id,
    uint16_t status
)
{
    char jsonResponseStr[100];
    methodResponseBuffer = (char*)malloc(100);
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
    az_span_to_str((char*)telemetry_payload, sizeof(telemetry_payload), temp_span);
    return (char*)telemetry_payload;
}