#ifndef GSM_UTILITIES_H
#define GSM_UTILITIES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t year;       // From 2000
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} gsm_utilities_date_time_t;


/**
 * @brief Get gsm imei from buffer
 * @param[in] IMEI raw buffer from gsm module
 * @param[out] IMEI result
 */ 
void gsm_utilities_get_imei(uint8_t *imei_buffer, uint8_t * result);

/**
 * @brief Get signal strength from buffer
 * @param[in] buffer buffer response from GSM module
 * @param[out] CSQ value
 * @note buffer = "+CSQ:..."
 * @retval TRUE Get csq success
 *         FALSE Get csq fail
 */ 
bool gsm_utilities_get_signal_strength_from_buffer(char * buffer, uint8_t * csq);

/**
 * @brief Get Number from string
 * @param[in] Index Begin index of buffer want to find
 * @param[in] buffer Data want to search
 * @note buffer = "abc124mff" =>> gsm_utilities_get_number_from_string(3, buffer) = 123
 * @retval Number result from string
 */ 
uint32_t gsm_utilities_get_number_from_string(uint32_t index, char* buffer);

/**
 * @brief Get Number from hexa string
 * @param[in] Index Begin index of buffer want to find
 * @param[in] buffer Data want to search
 * @note buffer = "abc124amff" =>> gsm_utilities_get_number_from_string(3, buffer) = 123a
 * @retval Number result from string
 */
uint32_t gsm_utilities_get_hexa_from_string(uint32_t index, char *buffer);
/**
 * @brief Get Number from buffer
 * @param[in] Index Begin index of buffer want to find
 * @param[in] buffer Data want to search
 * @param[in] length Buffer length
 * @note buffer = "abc124mff" =>> gsm_utilities_get_number_from_buffer(3, buffer) = 123
 * @retval Number result from string
 */ 
uint32_t gsm_utilities_get_number_from_buffer(char* buffer, uint8_t offset, uint8_t length);
  
/**
 * @brief Caculate CRC16
 * @param[in] data_p Data to caculate CRC16 value
 * @param[in] length Length of data
 * @retval CRC16 value
 */ 
uint16_t gsm_utilities_crc16(const uint8_t* data_p, uint8_t length);

/**
 * @brief get phone number from AT+CLIP message
 * @param[in] data : Pointer to buffer data will be parsed
 * @param[in] result : Phone number result
 * @note : Format input data "0942018895",129,"",0,"",0
 */ 
void gsm_utilities_get_phone_number_from_at_clip(char * data, char * result);

/**
 * @brief Parse HTTP action response
 * @param[in] response : Pointer to buffer data will be parsed
 * @param[out] error_code : HTTP response code
 * @param[out] content_length : HTTP response length
 * @note : Format data input "+HTTPACTION: 0,200,12314\r\n"
 * @retval TRUE  : Message is valid format, operation success
 *         FALSE : Message is invalid format, operation failed
 */ 
bool gsm_utilities_parse_http_action_response(char* response, uint32_t *error_code, uint32_t *content_length);

/**
 * @brief Parse HTTPRREAD message response size
 * @param[in] buffer : Pointer to buffer data will be parsed
 * @param[out] begin_data_pointer : Address of user data in buffer
 * @note : Format data input "+HTTPREAD: 123\r\nData...."
 * @retval -1  : GSM HTTPREAD message error
 *         Otherwise : HTTP DATA size
 */ 
int32_t gsm_utilities_parse_httpread_msg(char * buffer, uint8_t **begin_data_pointer);

/**
 * @brief Parse GSM HTTP timestamp response message
 * @param[in] buffer : Pointer to buffer data will be parsed
 * @param[out] date_time : Date time result
 * @note : Format message "\r\n+CCLK : "yy/MM/dd,hh:mm:ss+zz"\r\n\r\nOK\r\n"
 * @retval TRUE  : Operation success
 *         FALSE : Response message is invalid format or invalid timestamp
 */ 
bool gsm_utilities_parse_timestamp_buffer(char *buffer, void *date_time);

/**
 * @brief Parse GSM timezone buffer
 * @param[in] buffer : Pointer to buffer data will be parsed
 * @param[out] timezone : Timezone result
 * @note : Format message "+CTZV: +28,0\r\n"
 * @retval TRUE  : Operation success
 *         FALSE : Response message is invalid format or invalid timezone
 */ 
bool gsm_utilities_parse_timezone_buffer(char *buffer, int8_t *timezone);

/**
 * @brief Parse GSM timestamp psuttz message
 * @param[in] buffer : Pointer to buffer data will be parsed
 * @param[out] date_time : Datetime result
 * @param[out] gsm_adjust : Timezone
 * @note : Format message "*PSUTTZ: 2021,1,24,10,13,52,"+28",0\r\n"
 * @retval TRUE  : Operation success
 *         FALSE : Response message is invalid format or invalid timezone
 */ 
bool gps_utilities_parse_timestamp_from_psuttz_message(char * buffer, void*date_time, int8_t *gsm_adjust);

#endif /* GSM_UTILITIES_H */

