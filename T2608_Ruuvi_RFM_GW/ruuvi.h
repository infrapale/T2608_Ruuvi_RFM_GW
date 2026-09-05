#ifndef __RUUVI_H__
#define __RUUVI_H__
// #include "uart.h"

#define RUUVI_NBR_OF            3
#define RUUVI_RFM_INTERVAL      6000
#define RUUVI_DATA_POINTS       10
#define RUUVI_NAME_LEN          20
#define RUUVI_MAC_LEN           6
#define RUUVI_AVG_POINTS        10

typedef struct 
{
    char    name[RUUVI_NAME_LEN];
    uint8_t mac[RUUVI_MAC_LEN];
} ruuvi_const_data_st;

struct RuuviData {
    float temperature;
    float humidity;
    float pressure;
    float accelX;
    float accelY;
    float accelZ;
    float batteryVoltage;
    int8_t txPower;
    uint8_t movementCounter;
    uint16_t sequence;
};


typedef struct
{
    int8_t  tx_indx;
    // char    name[20];
    float   temp;
    float   hum;
    float   pressure;
    float   battery;
    int16_t tx_power;
    bool    updated;
    uint32_t    next_send;
} ruuvi_data_st; 

typedef struct
{
    bool        updated;
    uint8_t     save_indx;
    uint32_t    next_send;
} ruuvi_meta_st; 


void ruuvi_initialize(); 
#endif