/******************************************************************************
 * @file    	GSM_Utilities.c
 * @author  	
 * @version 	V1.0.0
 * @date    	15/01/2014
 * @brief   	
 ******************************************************************************/

/******************************************************************************
                                   INCLUDES					    			 
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "gsm_utilities.h"

#define GSM_IMEI_MAX_LENGTH     32

void gsm_utilities_get_imei(uint8_t *imei_buffer, uint8_t *result)
{
    uint8_t count = 0;
    uint8_t tmp_count = 0;

    for (count = 0; count < strlen((char *)imei_buffer); count++)
    {
        if (imei_buffer[count] >= '0' && imei_buffer[count] <= '9')
        {
            result[tmp_count++] = imei_buffer[count];
        }

        if (tmp_count >= GSM_IMEI_MAX_LENGTH)
        {
            break;
        }
    }

    result[tmp_count] = 0;
}

bool gsm_utilities_get_signal_strength_from_buffer(char *buffer, uint8_t *csq)
{
    // assert(buffer);

    char *temp = strstr((char *)buffer, "+CSQ:");

    if (temp == NULL)
        return false;

    *csq = gsm_utilities_get_number_from_string(6, temp);

    return true;
}

uint32_t gsm_utilities_get_number_from_string(uint32_t index, char *buffer)
{
    // assert(buffer);

    uint32_t value = 0;
    uint32_t tmp = index;

    while (buffer[tmp] && tmp < 1024)
    {
        if (buffer[tmp] >= '0' && buffer[tmp] <= '9')
        {
            value *= 10;
            value += buffer[tmp] - 48;
        }
        else
        {
            break;
        }
        tmp++;
    }

    return value;
}

// 0xAA
uint32_t gsm_utilities_get_hexa_from_string(uint32_t index, char *buffer)
{
    uint32_t value = 0;
    uint32_t tmp = index;
    while (buffer[tmp] && tmp < strlen(buffer))
    {
        if (buffer[tmp] >= '0' && buffer[tmp] <= '9')
        {
            value *= 16;
            value += buffer[tmp] - 48;
        }
        else if ((buffer[tmp] >= 'a' && buffer[tmp] <= 'f'))
        {
            value *= 16;
            value += buffer[tmp] - 'a' + 10;
        }
        else if ((buffer[tmp] >= 'A' && buffer[tmp] <= 'F'))
        {
            value *= 16;
            value += buffer[tmp] - 'a' + 10;
        }
        else
        {
            break;
        }
        tmp++;
    }

    return value;
}

uint32_t gsm_utilities_get_number_from_buffer(char *buffer, uint8_t offset, uint8_t length)
{
    uint32_t value = 0;
    uint8_t tmp_count = 0;

    value = 0;
    tmp_count = offset;
    length += offset;

    while (buffer[tmp_count] && tmp_count < length)
    {
        if (buffer[tmp_count] >= '0' && buffer[tmp_count] <= '9')
        {
            value *= 10;
            value += buffer[tmp_count] - 48;
        }
        else
        {
            break;
        }

        tmp_count++;
    }

    return value;
}

uint16_t gsm_utilities_crc16(const uint8_t *data_p, uint8_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }

    return crc;
}

// "0942018895",129,"",0,"",0
void gsm_utilities_get_phone_number_from_at_clip(char *data, char *result)
{
    *(result) = '\0';

    while (*data == '"')
    {
        data++;
    }

    while (*data != '"')
    {
        *result++ = *data++;
    }
}

// +HTTPACTION: 0,200,12314\r\n
bool gsm_utilities_parse_http_action_response(char *response, uint32_t *error_code, uint32_t *content_length)
{
    bool retval = false;
    char tmp[32];
    char *p;

    p = strstr(response, "+HTTPACTION: 0,200,");
    if (p)
    {
        p += strlen("+HTTPACTION: 0,200,");
        for (uint32_t i = 0; i < (sizeof(tmp) - 1); i++, p++)
        {
            if (*p != '\r')
            {
                tmp[i] = *p;
            }
            else
            {
                tmp[i] = '\0';
                break;
            }
        }

        *content_length = atoi(tmp);
        *error_code = 200;
        retval = true;
    }
    else
    {
        // TODO parse error code
        retval = false;
    }
    return retval;
}

int32_t gsm_utilities_parse_httpread_msg(char *buffer, uint8_t **begin_data_pointer)
{
    // +HTTPREAD: 123\r\nData
    char tmp[32];
    char *p = strstr(buffer, "+HTTPREAD: ");
    if (p == NULL)
    {
        return -1;
    }

    p += strlen("+HTTPREAD: ");
    if (strstr(p, "\r\n") == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < (sizeof(tmp) - 1); i++, p++)
    {
        if (*p != '\r')
        {
            tmp[i] = *p;
        }
        else
        {
            tmp[i] = '\0';
            break;
        }
    }
    p += 2; // Skip \r\n
    *begin_data_pointer = (uint8_t*)p;

    return atoi(tmp);
}

#if 0
//sys_ctx()->rtc->set_date_time(date_time, 0);
//DEBUG_PRINTF("Valid timestamp\n");
bool gsm_utilities_parse_timestamp_buffer(char *response_buffer, gsm_utilities_date_time_t *date_time)
{
    // Parse response buffer
    // "\r\n+CCLK : "yy/MM/dd,hh:mm:ss+zz"\r\n\r\nOK\r\n        zz : timezone
    // \r\n+CCLK : "10/05/06,00:01:52+08"\r\n\r\nOK\r\n
    bool val = false;
    uint8_t tmp[4];
    char *p_index = strstr(response_buffer, "+CCLK");
    if (p_index == NULL)
    {
        goto exit;
    }

    while (*p_index && ((*p_index) != '"'))
    {
        p_index++;
    }
    if (*p_index == '\0')
    {
        goto exit;
    }
    p_index++;
    response_buffer = p_index;

    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, response_buffer, 2);
    date_time->year = atoi(tmp);
    if (date_time->year < 20) // 2020
    {
        // Invalid timestamp
        val = false;
    }
    else
    {
        // MM
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, response_buffer + 3, 2);
        date_time->month = atoi(tmp);

        // dd
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, response_buffer + 6, 2);
        date_time->day = atoi(tmp);

        // hh
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, response_buffer + 9, 2);
        date_time->hour = atoi(tmp);

        // mm
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, response_buffer + 12, 2);
        date_time->minute = atoi(tmp);

        // ss
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, response_buffer + 15, 2);
        date_time->second = atoi(tmp);

        val = true;
    }

exit:
    return val;
}

bool gsm_utilities_parse_timezone_buffer(char *buffer, int8_t *timezone)
{
    // +CTZV: +28,0\r\n
    bool retval = false;
    int negative = -1;
    char *p;
    char timezone_str[3];

    p = strstr(buffer, "+CTZV: ");
    if (p && strstr(p, ","))
    {
        p += strlen("+CTZV: ");
        if (*p++ == '+')
        {
            negative = 1;
        }

        timezone_str[0] = *p++;
        if (*p != ',')
        {
            timezone_str[1] = *p;
            timezone_str[2] = '\0';
        }
        else
        {
            timezone_str[1] = '\0';
        }

        *timezone = negative * (atoi(timezone_str) / 4);
        retval = true;
    }
    else
    {
        goto end;
    }

end:
    return retval;
}

bool gps_utilities_parse_timestamp_from_psuttz_message(char *buffer, gsm_utilities_date_time_t *date_time, int8_t *gmt_adjust)
{
    // "*PSUTTZ: 2021,1,24,10,13,52,"+28",0\r\n"        // year,month,day,hour,min,sec,zone*4,idont know last params
    bool retval = false;
    char *p;

    p = strstr(buffer, "*PSUTTZ: ");
    if (p && strstr(p, "\r\n"))
    {
        int tmp_int, int_negative = -1, time_zone;
        char tmp_str[8];
        // get Year
        p += strlen("*PSUTTZ: ");

        // Year
        for (uint32_t i = 0; (i < 5) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }

        tmp_int = atoi(tmp_str);
        if (tmp_int < 2021)
        {
            goto end;
        }
        date_time->year = tmp_int - 2000;

        // Month
        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }
        tmp_int = atoi(tmp_str);
        if (tmp_int > 12 || tmp_int < 1)
        {
            goto end;
        }
        date_time->month = tmp_int;

        // Day
        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }
        tmp_int = atoi(tmp_str);
        if (tmp_int > 31 || tmp_int < 0)
        {
            goto end;
        }
        date_time->day = tmp_int;

        //Hour
        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }
        tmp_int = atoi(tmp_str);
        if (tmp_int > 24 || tmp_int < 0)
        {
            goto end;
        }
        date_time->hour = tmp_int;

        // Min
        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }
        tmp_int = atoi(tmp_str);
        if (tmp_int > 60 || tmp_int < 0)
        {
            goto end;
        }
        date_time->minute = tmp_int;

        // Sec
        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != ',')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }
        tmp_int = atoi(tmp_str);
        if (tmp_int > 60 || tmp_int < 0)
        {
            goto end;
        }
        date_time->second = tmp_int;

        // Zone
        p++; // Skip "\""
        if (*p++ == '+')
        {
            int_negative = 1;
        }
        else if (*p++ == '-')
        {
            int_negative = -1;
        }
        else
        {
            goto end;
        }

        for (uint32_t i = 0; (i < 3) && (*p); i++, p++)
        {
            tmp_str[i] = '\0';
            if (*p != '"')
            {
                tmp_str[i] = *p;
            }
            else
            {
                p++;
                break;
            }
        }

        *gmt_adjust = int_negative * (atoi(tmp_str) / 4);

        retval = true;
    }
    else
    {
        goto end;
    }

end:
    return retval;
}

#endif
