#include <Arduino.h>



// Azure IoT SDK for C includes



// Move to header if needed:
bool isNumeric(const char* s);


void receivedCallback(char* topic, byte* payload, unsigned int length);

bool DoMethod(char * method, char * payload);
char* get_Method_Response(az_span az_span_id, char * method, char * parameter, uint32_t id, uint16_t status); 


char* generateTelemetryPayload(uint32_t telemetry_send_count, int value);
az_iot_message_properties * GetProperties(int value);

AZ_NODISCARD az_result az_span_relaxed_atou32(az_span source, uint32_t* out_number);

char* getCurrentLocalTimeString();




// Publish 1 message every 2 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 10000

#define WARNING "Status=Warning Value less than 100"
#define NO_WARNING "Status=NO Warning Value greater 100"

//Used in AZ_NODISCARD az_result az_span_relaxed_atou32():
// Get char digits into an integer from the source as an az_span until char is not a digit
// Digits are HEX as per _az_NUMBER_OF_DECIMAL_VALUES
#define _az_NUMBER_OF_DECIMAL_VALUES 16

extern bool IsRunning;
extern bool LEDIsOn;
extern unsigned long TelemetryFrequencyMilliseconds ;
extern az_iot_hub_client client;
extern char telemetry_topic[128];
extern uint8_t telemetry_payload[1024];
extern char * methodResponseBuffer;
extern PubSubClient mqtt_client;
extern az_iot_message_properties properties;


// Nb: Here as it gets set when the Method start telemetry:
extern unsigned long next_telemetry_send_time_ms;