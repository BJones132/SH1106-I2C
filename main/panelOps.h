#pragma once

//SH1106 Command bytes
#define SH1106_CMD_SET_MULTIPLEX_RATIO 0xA8
#define SH1106_CMD_SET_COMPINS 0xDA //Set common signals pad configuration
#define SH1106_CMD_SET_DISP_ON 0xAF
#define SH1106_CMD_SET_DISP_OFF 0xAE
#define SH1106_CMD_SET_CHARGE_PUMP 0xAD
#define SH1106_CMD_SET_REVERSE_OFF 0xA6
#define SH1106_CMD_SET_REVERSE_ON 0xA7


//SH1106 Data bytes
//Some may technically be second command of double command bytes
#define SH1106_DATA_SET_MULTIPLEX_RATIO_DEFAULT 0x3F //Default 64 multiplex ratio
#define SH1106_DATA_SET_COMPINS_DEFAULT 0x12 //Default common signals pad configuration (Alternative)
#define SH1106_DATA_SET_CHARGE_PUMP_ON 0x8D
#define SH1106_DATA_SET_CHARGE_PUMP_OFF 0x8C