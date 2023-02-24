#ifndef az_local_h
#define az_local_h
#include <Arduino.h>

void Restart();

// Azure IoT SDK for C includes

struct Properties {
    unsigned long TelemetryFrequencyMilliseconds;
    bool IsRunning = false;
    bool LEDIsOn = false;
    bool fanOn = false;
};

//Nb: By default, Acknoweledgement is not required for CD Messages. So use none
#define ACK_MODE full
#define CD_MESSAGE_OUTCOME success

// Publish 1 message every 10 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 10000;

// Move to header if needed:
bool isNumeric(const char* s);


void receivedCallback(char* topic, byte* payload, unsigned int length);

void SetProperties( char * payload);
void UpdateProperties();

bool DoMethod(char * method, char * payload);
char* get_Method_Response(az_span az_span_id, char * method, char * parameter, uint32_t id, uint16_t status); 




AZ_NODISCARD az_result az_span_relaxed_atou32(az_span source, uint32_t* out_number);

char* getCurrentLocalTimeString();






#define WARNING "Status=Warning Value less than 100"
#define NO_WARNING "Status=NO Warning Value greater 100"

//Used in AZ_NODISCARD az_result az_span_relaxed_atou32():
// Get char digits into an integer from the source as an az_span until char is not a digit
// Digits are HEX as per _az_NUMBER_OF_DECIMAL_VALUES
#define _az_NUMBER_OF_DECIMAL_VALUES 16

extern bool isRestarting;
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

// 
#define IOT_SAMPLE_MQTT_PUBLISH_QOS 0
#define IOT_SAMPLE_MQTT_SUBSCRIBE_QOS 1

// https://eclipse.github.io/paho.mqtt.c/MQTTClient/html/_m_q_t_t_client_8h.html#:~:text=%E2%97%86%20MQTTCLIENT_SUCCESS%20%23define%20MQTTCLIENT_SUCCESS%200%20Return%20code%3A%20No,Indicates%20successful%20completion%20of%20an%20MQTT%20client%20operation.
#define MQTTCLIENT_SUCCESS   0
#define MQTTCLIENT_FAILURE   -1
#define MQTTCLIENT_DISCONNECTED   -3

void get_device_twin_document(void);
bool receive_device_twin_message(void);

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span const version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static az_span const desired_device_count_property_name = AZ_SPAN_LITERAL_FROM_STR("TelemetryFrequencyMilliseconds");
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// misc ////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool isNumeric(const char* s);
AZ_NODISCARD az_result az_span_relaxed_atou32(az_span source, uint32_t* out_number);

// az_local_telemetry  /////////////////////////////////////////////////////////////////////////////////////////////
char * generateTelemetryPayload(uint32_t telemetry_send_count, const char * propertyName, int value);
az_iot_message_properties* GetProperties(int value);


// az_local_methods /////////////////////////////////////////////////////////////////////////////////////////////////

bool DoMethod(
    char* method,
    char* payload
);

char* get_Method_Response(
    az_span az_span_id,
    char* method,
    char* parameter,
    uint32_t id,
    uint16_t status
);

// az_local_twin_and_properties ///////////////////////////////////////////////////////////////////////////////////////////////

enum CD_TWIN_PROPERTY_DATA_TYPE { DT_NULL, DT_INT, DT_BOOL, DT_DOUBLE, DT_STRING };



extern char  PropsJson[512];
extern bool GotTwinDoc;
extern struct Properties Dev_Properties;
extern int NumTabs;

extern bool MethodsSubscribed;
extern bool CDMessagesSubscribed;
extern bool TwinResponseSubscribed;
extern bool TwinPatchSubscribed;

void InitProperties(void);
void SaveProperties();
void LoadProperties();
void ReportProperties();
void PrintProperties();
void PrintStructProperties();
void get_device_twin_document(void);
void SetProperties( char * payload);
void SetHardware();
extern bool DoSetHardware;

// Desired Property Components
const char * const components[] = {"system","climate"};

//#define USING_ARDUINO_IDE




void send_reported_property(const char* propertyName, byte * propertyValue, uint8_t propertySize, CD_TWIN_PROPERTY_DATA_TYPE propertyType);

void build_reported_property_int(
    const char * desired_property_name,
    int32_t device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload);

#define NUM_REPORTED_DOUBLE_FRACTIONAL_DIGITS 4

void build_reported_property_double(
    const char * desired_property_name,
    double device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload);

void build_reported_property_bool(
    const char * desired_property_name,
    bool device_property_value,
    az_span reported_property_payload,
    az_span* out_reported_property_payload);

void build_reported_property_null(
    const char * desired_property_name,
    az_span reported_property_payload,
    az_span* out_reported_property_payload);


#endif