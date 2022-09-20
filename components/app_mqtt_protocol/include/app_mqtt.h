#ifndef APP_MQTT_T
#define APP_MQTT_T

#include <stdint.h>
#include <stdbool.h>


typedef union
{
    struct 
    {
        uint8_t gsm_internet : 1;
        uint8_t eth_internet : 1;
        uint8_t wifi_internet : 1;
        uint8_t reserve : 5;
    } __attribute__((packed)) name;
    uint8_t value;
} __attribute__((packed)) interface_internet_t;

typedef struct 
{
    /* data */
    uint16_t fire_status;
    uint8_t csq;
    int sensor_cnt;
    uint8_t batery;
    uint8_t networkInterface;
    uint8_t temper;
    uint32_t fire_zone;
    interface_internet_t networkStatus;
    bool inTestmode;
} __attribute__((packed)) fire_status_t;




#endif