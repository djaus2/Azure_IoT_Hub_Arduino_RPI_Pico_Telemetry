
#include <az_core.h>
#include <az_iot.h>
//#include <azure_ca.h>
#include <az_iot_hub_client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "az_local_msglevels.h"
#include "az_local.h"

#include <string.h>


bool isNumeric(const char* s) {
    while (*s) {
        if (*s < '0' || *s > '9')
            return false;
        ++s;
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


