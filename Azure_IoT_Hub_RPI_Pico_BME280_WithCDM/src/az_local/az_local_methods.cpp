#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local_msglevels.h"
#include "az_local.h"


#include <string.h>

void(*resetFunc) (void) = 0;

char* methodResponseBuffer;
DynamicJsonDocument methodResponseDoc(64);


//Perform the method with optional payload that is interpretted as an integer
bool DoMethod(char* method, char* payload)
{
    bool retv = true;
    PRINT_BEGIN_SUB_1("Doing Method");
    {
        SERIALPRINT("Method: ");
        SERIAL_PRINTLN(method);
        int value = -1;
        if (payload != NULL)
        {
            // Actually should interpret as Json (2Do), but for simplicity just as a number for now. ???
            if (strcmp(payload, "\"\"") == 0)
            {
                SERIALPRINTLN(" Mo Payload: ");
            }
            else if (strlen(payload) == 0)
            {
                SERIALPRINTLN(" Mo Payload: ");
            }
            else
            {
                SERIALPRINT(" Payload: ");
                SERIAL_PRINT(payload);

                if (isNumeric(payload))
                {
                    value = atoi(payload);
                    SERIAL_PRINT(" which is the number: ");
                    SERIAL_PRINTLN(value);
                }
                else if (strcmp(payload,"true")==0)
                {
                    value = 1;
                    SERIAL_PRINT(" which is the bool: ");
                    SERIAL_PRINTLN(value);
                }
                else if (strcmp(payload, "false")==0)
                {
                    value = 0;
                    SERIAL_PRINT(" which is the bool: ");
                    SERIAL_PRINTLN(value);
                }
                else if (strcmp(payload, "on") == 0)
                {
                    value = 1;
                    SERIAL_PRINT(" which is the bool: ");
                    SERIAL_PRINTLN(value);
                }
                else if (strcmp(payload, "off") == 0)
                {
                    value = 0;
                    SERIAL_PRINT(" which is the bool: ");
                    SERIAL_PRINTLN(value);
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
                            SERIALPRINT(" which isn't a number.");
                        }
                    }
                    else
                    {
                        SERIALPRINT(" which isn't a number.");
                    }
                }
            }
        }
        else
        {
            SERIALPRINTLN(" Mo Payload: ");
        }
        SERIALPRINTLN();
        Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
        Serial.print("Direct Method: ");
        Serial.println(method);
        if (strncmp(method, "reset", 4) == 0)
        {
            Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
            Serial.println("Device is restarting in 5");
            Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");

            for (int i = 5; i > -1; i--)
            {
                Serial.println(i);
                delay(1000);
            }
            Serial.println("Restarting");
            delay(500);
            //resetFunc(); <-- This did not work!
            Restart();
        }
        else if (strncmp(method, "start", 4) == 0)
        {
            if (!Dev_Properties.IsRunning)
            {
                if (Dev_Properties.TelemetryFrequencyMilliseconds != 0)
                {
                    next_telemetry_send_time_ms = millis();
                    Dev_Properties.IsRunning = true;

                    Serial.println("Telemtry was started.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );

                    SaveProperties();
                }
                else
                {
                    Serial.println("Telemetry is still stopped as period set to 0");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
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
                Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                SaveProperties();
            }
            else
            {
                Serial.println("Telemtry was already stopped.");
                Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
            }
        }
        else if (strncmp(method, "frequency", 4) == 0)
        {
            if (value < 0)
            {
                Serial.println(("Invalid Telemetry Period value. Should be no. seconds."));
                Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                return false;
            }
            else
            {
                Dev_Properties.TelemetryFrequencyMilliseconds = value * 1000;
                if (value == 0)
                {
                    Dev_Properties.IsRunning = false;
                    Serial.println("Telemtry is stopped.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                }
                else
                {
                    Serial.print("Telemetry Period is now: ");
                    Serial.print(value);
                    Serial.println(" sec.");
                    Dev_Properties.IsRunning = true;
                    Serial.println("Telemtry is running.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                }
                SaveProperties();
            }
        }
        else if (strncmp(method, "toggle", 4) == 0)
        {
            Dev_Properties.LEDIsOn = !Dev_Properties.LEDIsOn;
            if (Dev_Properties.LEDIsOn)
            {
                digitalWrite(LED_BUILTIN, HIGH);
                Dev_Properties.LEDIsOn = true;
                Serial.println("LED is On.");
                Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
            }
            else
            {
                digitalWrite(LED_BUILTIN, LOW);
                Dev_Properties.LEDIsOn = false;
                Serial.println("LED is Off.");
                Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
            }
            SaveProperties();
            SERIALPRINT("LED Toggled.");
        }
        else if (strncmp(method, "fanOn", 4) == 0)
        {
            if(Dev_Properties.fanOn)
            {
                if (value == 0)
                {
                    Dev_Properties.fanOn = false;
                    Serial.println("Fan turned Off.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                    SaveProperties();
                    SERIALPRINT("Fan Off.");
                }
                else
                {
                    Serial.println("Fan Unchanged.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                    SERIALPRINT("Fan unchanged.");
                }
            }
            else
            {
                if (value == 1)
                {
                    Dev_Properties.fanOn = true;
                    Serial.println("Fan turned On.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                    SaveProperties();
                    SERIALPRINT("Fan On.");
                }
                else
                {
                    Serial.println("Fan Unchanged.");
                    Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
                    SERIALPRINT("Fan Unchanged.");
                }

            }
            
        }
        else if (strncmp(method, "print", 4) == 0)
        {
            if (value == 0)
            {
                PrintStructProperties();
            }
            if (value == 1)
            {
                PrintProperties();
            }
            else
            {
                PrintStructProperties();
            }
        }
        else if (strncmp(method, "subscribe", 4) == 0)
        {
            if (value == 0)
            {
                if (MethodsSubscribed == false)
                {
                    mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
                    MethodsSubscribed = true;
                    SERIALPRINTLN("CD METHODS turned ON."); // Best to set to on at bootup.
                    SERIALPRINTLN("CATCH 22: You need you have done this to do this!");
                }
            }
            else if (value == 1)
            {
                if (CDMessagesSubscribed == false)
                {
                    mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
                    CDMessagesSubscribed = true;
                    SERIALPRINTLN("CD MESSAGES turned ON.");
                }
            }
            else  if (value == 2)
            {
                if (TwinResponseSubscribed == false)
                {
                    mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC);
                    TwinResponseSubscribed = true;
                    SERIALPRINTLN("TWIN GET RESPONSES SUBSCRIBED.");
                }
            }
            else  if (value == 3)
            {
                if (TwinPatchSubscribed == false)
                {
                    mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);
                    TwinPatchSubscribed = true;
                    SERIALPRINTLN("TWIN PATCHES RESPONSES SUBSCRIBED.");
                }
            }
            else
            {
                SERIALPRINTLN("Invalid Subscription to turn on.");
            }
        }
        else if (strncmp(method, "unsubscribe", 4) == 0)
        {
            if (value == 0)
            {
                if (MethodsSubscribed == true)
                {
                    mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
                    MethodsSubscribed = false;
                    SERIALPRINTLN("CD METHODS turned OFF.");
                    SERIALPRINTLN("WARNING: Only way to udo this is to reboot!");
                }
            }
            else  if (value == 1)
            {
                if (MethodsSubscribed == true)
                {
                    mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
                    CDMessagesSubscribed = false;
                    SERIALPRINTLN("CD MESSAGES turned OFF.");
                }
            }
            else  if (value == 2)
            {
                if (TwinResponseSubscribed == true)
                {
                    mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC);
                    TwinResponseSubscribed = false;
                    SERIALPRINTLN("TWIN GET RESPONSES UN-SUBSCRIBED.");
                }
            }
            else  if (TwinPatchSubscribed == 3)
            {
                if (MethodsSubscribed == true)
                {
                    mqtt_client.unsubscribe(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);
                    TwinPatchSubscribed = false;
                    SERIALPRINTLN("TWIN PATCHES RESPONSES UN-SUBSCRIBED.");
                }
            }
            else
            {
                SERIALPRINTLN("Invalid Subscription to turn off.");
                retv = false;
            }
        }
        else
        {
            Serial.println("Unrecognised Method: ");
            Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" );
            SERIALPRINTLN("Unrecognised Method: ");
            retv = false;
        }
    }
    PRINT_END_SUB_1
    return retv;
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
    PRINT_BEGIN_SUB_1(" Get Method Response: ")
    {
        char jsonResponseStr[256];
        methodResponseBuffer = (char*)malloc(256);
        az_result rc = az_iot_hub_client_methods_response_get_publish_topic(
            &client, //&hub_client,
            az_span_id,
            status,
            methodResponseBuffer,
            256,
            NULL);
        if (az_result_failed(rc))
        {
            Serial.println("Failed: az_iot_hub_client_methods_response_get_publish_topic");
        }
        DynamicJsonDocument methodResponseDoc2(128);
        methodResponseDoc2["request_id"] = id;
        methodResponseDoc2["method"] = method;
        methodResponseDoc2["parameter"] = parameter;
        serializeJson(methodResponseDoc2, jsonResponseStr);
        az_span temp_span = az_span_create_from_str(jsonResponseStr);
        SERIALPRINT("Json Method Response String: ")
        SERIAL_PRINTLN(jsonResponseStr);
        az_span_to_str((char*)telemetry_payload, sizeof(telemetry_payload), temp_span);
        return (char*)telemetry_payload;
    }
    PRINT_END_SUB_1
}