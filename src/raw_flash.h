#pragma once

#include "wiced_platform.h"
#include "wiced_hal_sflash.h"

//#define FLASH_ADDR (FLASH_SIZE - FLASH_SECTOR_SIZE)
//#define FLASH_ADDR_CNT (FLASH_SIZE - 2 * FLASH_SECTOR_SIZE)
#define FLASH_ADDR                    (FLASH_SIZE - FLASH_SECTOR_SIZE)
#define FLASH_ADDR_CNT                (FLASH_ADDR - FLASH_SECTOR_SIZE)
#define FLASH_ADDR_STATE              (FLASH_ADDR_CNT - FLASH_SECTOR_SIZE)
#define FLASH_ADDR_XXTEA_SN           (0xFC000)
#define DID_NEW_TYPE

#ifndef DID_NEW_TYPE
#define FLASH_XXTEA_SN_LEN           34        //byte
#define FLASH_XXTEA_ID_OFFSET        0         //DID在SN中的偏移
#define FLASH_XXTEA_ID_LEN           18        //byte
#define FLASH_XXTEA_KEY_OFFSET       18        //ID和KEY直接由一个逗号分隔符
#define FLASH_XXTEA_KEY_LEN          16        //byte
#else
#define FLASH_XXTEA_SN_LEN           53        //byte
#define FLASH_XXTEA_ID_OFFSET        0         //DID在SN中的偏移
#define FLASH_XXTEA_ID_LEN           20        //byte
#define FLASH_XXTEA_KEY_OFFSET       21        //ID和KEY直接由一个逗号分隔符
#define FLASH_XXTEA_KEY_LEN          32        //byte
#endif

// #define FLASH_ADDR_XXTEA_KEY          (FLASH_ADDR_XXTEA_SN+FLASH_XXTEA_SN_LEN)
// #define FLASH_ADDR_XXTEA_KEY_LEN      16
#define FLASH_ADDR_BINDKEY            (FLASH_ADDR_XXTEA_SN - FLASH_SECTOR_SIZE)

typedef struct
{
    uint8_t onoff_state;
    uint8_t led_state;
    uint8_t led_num;
} onoff_app_data_t;

extern void flash_app_init(void);

extern uint32_t flash_app_read_mem(uint32_t read_from, uint8_t *buf, uint32_t len);

extern uint32_t flash_app_write_mem(uint32_t write_to, uint8_t *data, uint32_t len);

extern uint32_t flash_write(uint32_t addr, uint8_t *data, uint32_t len);

extern uint32_t flash_write_erase(uint32_t addr, uint8_t *data, uint32_t len,uint8_t erase);

extern uint32_t flash_app_read(uint32_t addr, uint8_t *data, uint32_t len);
