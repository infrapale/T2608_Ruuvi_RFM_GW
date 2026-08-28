#include <Arduino.h>
#include <btstack.h>
#include "main.h"
#include "ruuvi.h"
#include "io.h"
//#include "uart.h"

#define  RUUVI_NBR_OF           2
#define  RUUVI_TX_INTERVAL      30000
// -----------------------------
// RuuviTag Format 5 Parser
// -----------------------------

typedef struct 
{
    uint8_t index;
    char    buff[UART_MSG_LEN];
} ruuvi_st;

ruuvi_st ruuvi = {0};

const uint8_t ruuvit [RUUVI_NBR_OF][6] =
{
    {0xE6,0x2C,0x8D,0xDB,0x22,0x35},
    {0xF2,0x5B,0x48,0x64,0x65,0x24,}
};

ruuvi_data_st ruuvi_data[RUUVI_NBR_OF] =
{
    {.tx_indx = -1, .name = "MH1", .temp = 0.0, .hum = 0.0, .pressure=0.0, .battery= 0.0, .tx_power=0, .updated=false},
    {.tx_indx = -1, .name = "Parvi", .temp = 0.0, .hum = 0.0, .pressure=0.0, .battery= 0.0, .tx_power=0, .updated=false},
};


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


void ruuvi_send_data(ruuvi_data_st *rdata)
{
    // <##B1T1=;Abcdef;21.4;44.0;2.9;>
    sprintf(ruuvi.buff,"<##B1T1=;%s;%.1f;%.0f;%.2f>",
        rdata->name,
        rdata->temp,
        rdata->hum,
        rdata->battery);
    Serial.println(ruuvi.buff);
    //uart_add_msg(rdata->tx_indx, ruuvi.buff);
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
                        if(addr[n] != ruuvit[rindx][n]) addr_match = false;
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
                    ruuvi_data[ruuvi.index].temp        = rd.temperature;
                    ruuvi_data[ruuvi.index].hum         = rd.humidity;
                    ruuvi_data[ruuvi.index].pressure    = rd.pressure;
                    ruuvi_data[ruuvi.index].battery     = rd.batteryVoltage;
                    ruuvi_data[ruuvi.index].tx_power    = rd.txPower;
                    ruuvi_data[ruuvi.index].updated     = true;
                    ruuvi_send_data(&ruuvi_data[ruuvi.index]);
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

    Serial.println("Scanning for RuuviTags...");
}

