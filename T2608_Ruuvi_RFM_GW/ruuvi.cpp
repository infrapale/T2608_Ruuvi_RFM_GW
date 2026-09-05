#include <Arduino.h>
#include <btstack.h>
#include "main.h"
#include "ruuvi.h"
#include "io.h"
#include "atask.h"
#include "r69.h"
//#include "uart.h"


// -----------------------------
// RuuviTag Format 5 Parser
// -----------------------------

typedef struct 
{
    uint8_t index;
    char    buff[UART_MSG_LEN];
} ruuvi_st;

void ruuvi_task(void);
//                                  123456789012345   ival  next  state  prev  cntr flag  call backup
atask_st ruuvi_th            =    {"Ruuvi Task     ", 1000,    0,     0,  255,    0,  1,  ruuvi_task };


ruuvi_st ruuvi = {0};

ruuvi_const_data_st ruuvit[RUUVI_NBR_OF] =
{
    { .name ="MH1", .mac ={0xE6,0x2C,0x8D,0xDB,0x22,0x35}},
    { .name ="Parvi", .mac ={0xF2,0x5B,0x48,0x64,0x65,0x24}},
    { .name ="Parveke", .mac ={0xEA,0x78,0xE2,0x12,0x36,0xF8}}
};

ruuvi_meta_st ruuvi_meta[RUUVI_NBR_OF] = {0};

ruuvi_data_st ruuvi_rd_data = {0};

ruuvi_data_st ruuvi_data[RUUVI_NBR_OF] = {0};

ruuvi_data_st ruuvi_cum_data[RUUVI_NBR_OF] = {0};


bool parseRuuviFormat5(const uint8_t *data, uint8_t len, RuuviData &out) {
    if (len < 24) return false;

    if (data[0] != 0x99 || data[1] != 0x04) return false;
    if (data[2] != 0x05) return false;

    int16_t tempRaw = (data[3] << 8) | data[4];
    uint16_t humRaw = (data[5] << 8) | data[6];
    uint16_t presRaw = (data[7] << 8) | data[8];
    int16_t axRaw = (data[9] << 8) | data[10];
    int16_t ayRaw = (data[11] << 8) | data[12];
    int16_t azRaw = (data[13] << 8) | data[14];
    uint16_t powerInfo = (data[15] << 8) | data[16];

    out.temperature = tempRaw * 0.005f;
    out.humidity = humRaw * 0.0025f;
    out.pressure = (presRaw + 50000) / 100.0f;

    out.accelX = axRaw / 1000.0f;
    out.accelY = ayRaw / 1000.0f;
    out.accelZ = azRaw / 1000.0f;

    out.batteryVoltage = ((powerInfo >> 5) + 1600) / 1000.0f;
    out.txPower = (powerInfo & 0x1F) * 2 - 40;

    out.movementCounter = data[17];
    out.sequence = (data[18] << 8) | data[19];

    return true;
}

// -----------------------------
// BLE Scanner
// -----------------------------
static btstack_packet_callback_registration_t hci_cb;


void ruuvi_save_data(uint8_t rindx, ruuvi_data_st *ruuvi_rd_data)
{

    ruuvi_cum_data[rindx].temp     += ruuvi_rd_data->temp;
    ruuvi_cum_data[rindx].hum      += ruuvi_rd_data->hum;
    ruuvi_cum_data[rindx].pressure += ruuvi_rd_data->pressure;
    ruuvi_cum_data[rindx].battery  += ruuvi_rd_data->battery;

    if(++ruuvi_meta[rindx].save_indx >= RUUVI_AVG_POINTS) {
        ruuvi_data[rindx].temp = ruuvi_cum_data[rindx].temp / RUUVI_AVG_POINTS;
        ruuvi_data[rindx].hum = ruuvi_cum_data[rindx].hum / RUUVI_AVG_POINTS;
        ruuvi_data[rindx].pressure = ruuvi_cum_data[rindx].pressure / RUUVI_AVG_POINTS;
        ruuvi_data[rindx].battery = ruuvi_cum_data[rindx].battery / RUUVI_AVG_POINTS;
        ruuvi_meta[rindx].save_indx = 0;
        ruuvi_meta[rindx].updated = true;

        ruuvi_cum_data[rindx].temp = 0.0;
        ruuvi_cum_data[rindx].hum = 0.0;
        ruuvi_cum_data[rindx].pressure = 0.0;
        ruuvi_cum_data[rindx].battery = 0.0;
    }
}

//                    
void ruuvi_send_data(uint8_t rindx)
{
    // <S;#;Abcdef;T;21.4;H;44.0;B;2.9;>
    sprintf(ruuvi.buff,"<S;%s;T;%.1f;H;%.0f;B;%.2f>",
        ruuvit[rindx].name,
        ruuvi_data[rindx].temp,
        ruuvi_data[rindx].hum,
        ruuvi_data[rindx].battery);
    Serial.println(ruuvi.buff);
    r69_send(ruuvi.buff);
    //uart_add_msg(rdata->tx_indx, ruuvi.buff);
}

void ruuvi_task(void)
{

    switch(ruuvi_th.state)
    {
        case 0:
            ruuvi_th.state = 10;
            break;
        case 10:
            if(ruuvi_meta[ruuvi.index].updated){
                if(millis() > ruuvi_meta[ruuvi.index].next_send){
                    ruuvi_meta[ruuvi.index].next_send = millis() + RUUVI_RFM_INTERVAL;
                    ruuvi_meta[ruuvi.index].updated = false;
                    ruuvi_send_data(ruuvi.index);
                }
            }
            ruuvi_th.state = 100;
            break;
        case 20:
            ruuvi_th.state = 10;
            break;
        case 30:
            ruuvi_th.state = 10;
            break;
        case 50:
            ruuvi_th.state = 10;
            break;
        case 100:
            if(++ruuvi.index >= RUUVI_NBR_OF) ruuvi.index++;
            ruuvi_th.state = 10;
            break;
    }
}

void handle_adv(uint8_t *packet) {
    uint8_t addr[6];

    gap_event_advertising_report_get_address(packet, addr);

    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    uint8_t len = gap_event_advertising_report_get_data_length(packet);
    //Debug: print every advertisement
    // Serial.print("ADV from ");
    // for (int i = 0; i < 6; i++) {
    //     Serial.printf("%02X", addr[i]);
    //     if (i < 5) Serial.print(":");
    // }
    // Serial.print("  len=");
    // Serial.println(len);

    // Look for Ruuvi manufacturer ID
    for (int i = 0; i < len - 2; i++) {
        if (data[i] == 0x99 && data[i+1] == 0x04) {
            RuuviData rd;
            if (parseRuuviFormat5(&data[i], len - i, rd)) {
                uint8_t rindx = 99;
                bool tuttu_ruuvi = false;
                for (rindx = 0; (rindx < RUUVI_NBR_OF) && !tuttu_ruuvi; rindx++) {
                    bool addr_match = true;
                    for (int n = 0; (n < 6) && addr_match; n++) {
                        if(addr[n] != ruuvit[rindx].mac[n]) addr_match = false;
                    }
                    if (addr_match) {
                        ruuvi.index = rindx;
                        tuttu_ruuvi = true;
                    }
                }

                if(tuttu_ruuvi){
                    // Serial.println("=== RuuviTag Found ===");
                    // Serial.println(ruuvi_data[ruuvi.index].name);
                    // Serial.printf("Temp: %.2f C\n", rd.temperature);
                    // Serial.printf("Hum: %.2f %%\n", rd.humidity);
                    // Serial.printf("Pressure: %.2f hPa\n", rd.pressure);
                    // Serial.printf("Accel: %.3f %.3f %.3f g\n", rd.accelX, rd.accelY, rd.accelZ);
                    // Serial.printf("Battery: %.3f V\n", rd.batteryVoltage);
                    // Serial.printf("TX Power: %d dBm\n", rd.txPower);
                    // Serial.printf("Movement: %u\n", rd.movementCounter);
                    // Serial.printf("Sequence: %u\n", rd.sequence);
                    // Serial.println("======================\n");
                    ruuvi_rd_data.temp        = rd.temperature;
                    ruuvi_rd_data.hum         = rd.humidity;
                    ruuvi_rd_data.pressure    = rd.pressure;
                    ruuvi_rd_data.battery     = rd.batteryVoltage;
                    ruuvi_rd_data.tx_power    = rd.txPower;
                    ruuvi_rd_data.updated     = true;
                    ruuvi_save_data(ruuvi.index, &ruuvi_rd_data);
                    // ruuvi_send_data(ruuvi.index);
                }
            }
        }
    }
}

void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (type != HCI_EVENT_PACKET) return;

    uint8_t event = hci_event_packet_get_type(packet);

    if (event == GAP_EVENT_ADVERTISING_REPORT) {
        handle_adv(packet);
    }
}

void ruuvi_initialize() {

    Serial.println("Initializing BLE...");
    for (uint8_t i = 0; i < RUUVI_NBR_OF; i++)
    {
        ruuvi_meta[i].next_send = millis() + RUUVI_RFM_INTERVAL;
        ruuvi_meta[i].save_indx = 0;
        ruuvi_meta[i].updated = false;

        //ruuvi_data[i].tx_indx = uart_reserve_tx_buffer(RUUVI_TX_INTERVAL);
        
    }

    // BLE and Wi-Fi cannot run together
    //cyw43_arch_disable_wifi();

    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);

    hci_power_control(HCI_POWER_ON);

    // REQUIRED: start scanning
    gap_set_scan_parameters(0, 0x30, 0x30);
    gap_start_scan();

    atask_add_new(&ruuvi_th); 
    Serial.println("Scanning for RuuviTags...");
}

