/*****************************************************************************
T2608_Ruuvi_RFM_GWt 
*******************************************************************************

HW: Pi Pico 2W + RFM69

Receive Ruuvi Tag data and send via UART

??Optional: Receive RFM69 Date and time and send via UART

*******************************************************************************
https://github.com/infrapale/T2310_RFM69_TxRx
https://learn.adafruit.com/adafruit-feather-m0-radio-with-rfm69-packet-radio
https://learn.sparkfun.com/tutorials/rfm69hcw-hookup-guide/all
*******************************************************************************

*******************************************************************************

*******************************************************************************
**/

#include <Arduino.h>
#include "main.h"
#include "secrets.h"
#include "atask.h"
#include "io.h"
//#include "uart.h"
#include "handler.h"
#include "ruuvi.h"

//*********************************************************************************************
#define SERIAL_BAUD   9600
#define IO_TICK_INTERVAL    (100)

main_ctrl_st ctrl = {0};

void debug_print_task(void);
void rfm_receive_task(void); 

atask_st debug_print_handle        = {"Debug Print    ", 5000,0, 0, 255, 0, 1, debug_print_task};

void initialize_tasks(void)
{
    atask_initialize();
    atask_add_new(&debug_print_handle);
    //uart_initialize();
    handler_initialize();
    ruuvi_initialize();
}


void setup() 
{
    //while (!Serial); // wait until serial console is open, remove if not tethered to computer
    delay(2000);
    Serial.begin(9600);

    // SerialTFT.setTX(PIN_TX0 );   // UART0
    // SerialTFT.setRX(PIN_RX0);

    Serial.print(__APP__); Serial.print(F(" Compiled: "));
    Serial.print(__DATE__); Serial.print(" ");
    Serial.print(__TIME__); Serial.println();
    //SerialTFT.begin(9600);
    io_initialize();
    initialize_tasks();
}

void setup1(){
    io_initialize();
    ctrl.next_io_tick = millis() + IO_TICK_INTERVAL;
}


void loop() 
{
    atask_run();  
}

void loop1()
{
    if(millis() > ctrl.next_io_tick){
        ctrl.next_io_tick = millis() + IO_TICK_INTERVAL;
        io_task();
    }
}


void run_100ms(void)
{
    io_task();
}

void debug_print_task(void)
{
    atask_print_status(true);
}

