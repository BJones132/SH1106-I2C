#pragma once

//SH1106 Command bytes
#define SH1106_CMD_SET_MULTIPLEX_RATIO 0xA8
#define SH1106_CMD_SET_COMPINS 0xDA //Set common signals pad configuration
#define SH1106_CMD_SET_MIRROR_X_ON 0xA1
#define SH1106_CMD_SET_MIRROR_X_OFF 0xA0
#define SH1106_CMD_SET_DISP_ON 0xAF
#define SH1106_CMD_SET_DISP_OFF 0xAE
#define SH1106_CMD_SET_CHARGE_PUMP 0xAD
#define SH1106_CMD_SET_REVERSE_OFF 0xA6
#define SH1106_CMD_SET_REVERSE_ON 0xA7

#define SH1106_CMD_SET_LOWER_COLUMN_ADDR(num) (0x00 + num)
#define SH1106_CMD_SET_UPPER_COLUMN_ADDR(num) (0x10 + num)
#define SH1106_CMD_SET_PAGE_ADDR(num) (0xB0 + num)

#define SH1106_CMD_BEGIN_RWM 0xE0 //Begin Read-Write-Modify loop (Stores column addr, each write increments column addr)
#define SH1106_CMD_END_RWM 0xEE //End Read-Write-Modify loop (Column addr returns to addr at RWM begin)

//SH1106 Command Data bytes
#define SH1106_CMD_DATA_SET_MULTIPLEX_RATIO_DEFAULT 0x3F //Default 64 multiplex ratio
#define SH1106_CMD_DATA_SET_COMPINS_DEFAULT 0x12 //Default common signals pad configuration (Alternative)
#define SH1106_CMD_DATA_SET_CHARGE_PUMP_ON 0x8D
#define SH1106_CMD_DATA_SET_CHARGE_PUMP_OFF 0x8C

//SH1106 Data bytes