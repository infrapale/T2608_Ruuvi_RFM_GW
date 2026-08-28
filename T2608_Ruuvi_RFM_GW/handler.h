#ifndef __HANDLER_H__
#define __HANDLER_H__
#define FIELD_LEN   8
#define NBR_OF_NODES  4

void handler_initialize(void);

void handler_print_fields(void);

bool handler_split_msg(char *msg, int16_t rssi );

bool handler_parse_msg(char *msg, int16_t rssi );

void handler_debug_print(void);

#endif