#include <Arduino.h>



// Azure IoT SDK for C includes



// Move to header if needed:
bool isNumeric(const char* s);


void receivedCallback(char* topic, byte* payload, unsigned int length);

void SetProperties( char * payload);
void UpdateProperties();

bool DoMethod(char * method, char * payload);
char* get_Method_Response(az_span az_span_id, char * method, char * parameter, uint32_t id, uint16_t status); 




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

struct Properties {
  unsigned long TelemetryFrequencyMilliseconds = TELEMETRY_FREQUENCY_MILLISECS;
  bool MethodsSubscribed = false;
  bool CDMessagesSubscribed = false;
  bool IsRunning = false;
  bool LEDIsOn = false;
  bool fanOn = false;
};

extern char  PropsJson[512];
extern bool GotTwinDoc;
extern struct Properties Dev_Properties;
extern int NumTabs;

void InitProperties(void);
void SaveProperties();
void LoadProperties();
void ReportProperties();
void PrintProperties();
void PrintStructProperties();
void get_device_twin_document(void);
void SetProperties( char * payload);

// Desired Property Components
const char * const components[] = {"system","climate"};




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

/////////////////////////////

//#define MAX(a,b) (((a)>(b))?(a):(b))  ..is defined elsewhere

#define InsertTabs(_numtabs_) for (int i=0;i<_numtabs_;i++)Serial.print('\t')

#define SERIALPRINT(A) { InsertTabs(NumTabs);Serial.print(A);}
#define SERIALPRINTLN(A) { InsertTabs(NumTabs);Serial.println(A);}

#define _PRINT_BEGIN_SUB(_noTabs_,_A_,_ch_,_PLENGTH_) \
{ \
int NumTabs = _noTabs_;if (NumTabs == 0) { Serial.println(); } \
int PLENGTH = _PLENGTH_; \
char __ch__=_ch_; \
InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }Serial.println(); \
InsertTabs(NumTabs); Serial.println(_A_); \
InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }Serial.println(); \


#define _PRINT_END_SUB(_noTabs_) \
if (NumTabs != _noTabs_) {Serial.println("Number of tabs don't match!");} \
InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }Serial.println(); \
} \

#define PRINT_BEGIN(A_) _PRINT_BEGIN_SUB(0,A_,'=',80);
#define PRINT_END(B) Serial.print(" - ");Serial.print(B);Serial.println(); for (int i=0;i<PLENGTH;i++){Serial.print('=');}Serial.println();}

#define PRINT_BEGIN_SUB_1(A_) _PRINT_BEGIN_SUB(1,A_,'*',60);
#define PRINT_END_SUB_1 _PRINT_END_SUB(1);

#define PRINT_BEGIN_SUB_2(A_) _PRINT_BEGIN_SUB(2,A_,'-',60);
#define PRINT_END_SUB_2 _PRINT_END_SUB(2);

#define PRINT_BEGIN_SUB_3(A_) _PRINT_BEGIN_SUB(3,A_,'.',50);
#define PRINT_END_SUB_3 _PRINT_END_SUB(3);

#define PRINT_BEGIN_SUB_4(A_) _PRINT_BEGIN_SUB(4,A_,'_',40);
#define PRINT_END_SUB_4 _PRINT_END_SUB(4);

#define PRINT_BEGIN_SUB_5(A_) _PRINT_BEGIN_SUB(5,A_,'+',30);
#define PRINT_END_SUB_5 _PRINT_END_SUB(5);


/*
#define PRINT_BEGIN(A) {int NumTabs=0;int PLENGTH = 80; Serial.println();for (int i=0;i<PLENGTH;i++){Serial.print('=');}Serial.println();Serial.print(A);Serial.println();for (int i=0;i<PLENGTH;i++){Serial.print('=');}Serial.println();
#define PRINT_END(B) Serial.print(" - ");Serial.print(B);Serial.println(); for (int i=0;i<PLENGTH;i++){Serial.print('=');}Serial.println();}
#define PRINT_BEGIN_SUB_1(A) {int NumTabs=1;Serial.print("\t");int QLENGTH = 60;for (int i=0;i<QLENGTH;i++){Serial.print('_');}Serial.println();Serial.print("\t");Serial.println(A);Serial.print("\t");for (int i=0;i<QLENGTH;i++){Serial.print('_');}Serial.println();
#define PRINT_END_SUB_1 Serial.print("\t");for (int i=0;i<QLENGTH;i++){Serial.print('_');}Serial.println();}
#define PRINT_BEGIN_SUB_2(A) {int NumTabs=2;Serial.print("\t\t");int RLENGTH = 50;for (int i=0;i<RLENGTH;i++){Serial.print('.');}Serial.println();Serial.print("\t\t");Serial.println(A);Serial.print("\t\t");for (int i=0;i<RLENGTH;i++){Serial.print('.');}Serial.println();
#define PRINT_END_SUB_2 Serial.print("\t\t");for (int i=0;i<RLENGTH;i++){Serial.print('.');}Serial.println();}

#define PRINT_BEGIN_SUB_3(A) { int NumTabs=3;Serial.print("\t\t\t");int SLENGTH = MAX(strlen(A),40); for (int i = 0; i < SLENGTH; i++) { Serial.print('~'); }Serial.println(); Serial.print("\t\t\t");Serial.println(A); Serial.print("\t\t\t");for (int i=0;i<SLENGTH;i++){Serial.print('~');}Serial.println();

#define PRINT_END_SUB_3 Serial.print("\t\t\t");for (int i=0;i<SLENGTH;i++){Serial.print('~');}Serial.println();}

#define PRINT_BEGIN_SUB_4(A) { NumTabs=4;Serial.print("\t\t\t\t");int TLENGTH = MAX(strlen(A),30); for (int i = 0; i < TLENGTH; i++) { Serial.print(','); }Serial.println(); Serial.print("\t\t\t\t");Serial.println(A); Serial.print("\t\t\t\t");for (int i=0;i<TLENGTH;i++){Serial.print(',');}Serial.println();

#define PRINT_END_SUB_4 Serial.print("\t\t\t\t");for (int i=0;i<TLENGTH;i++){Serial.print(',');}Serial.println();}

#define PRINT_BEGIN_SUB_5(A) {NumTabs=5;Serial.print("\t\t\t\t\t");int ULENGTH = MAX(strlen(A),20); for (int i = 0; i < ULENGTH; i++) { Serial.print('+'); }Serial.println();  Serial.print("\t\t\t\t\t");Serial.println(A); Serial.print("\t\t\t\t\t");for (int i = 0; i < ULENGTH; i++) { Serial.print('+'); }Serial.println();

#define PRINT_END_SUB_5 Serial.print("\t\t\t\t\t");for (int i=0;i<ULENGTH;i++){Serial.print('+');}Serial.println();}
*/
