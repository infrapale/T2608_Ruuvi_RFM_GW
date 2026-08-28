#ifndef __RUUVI_H__
#define __RUUVI_H__
// #include "uart.h"

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
    char    name[20];
    float   temp;
    float   hum;
    float   pressure;
    float   battery;
    int16_t tx_power;
    bool    updated;
} ruuvi_data_st; 


void ruuvi_initialize(); 
#endif