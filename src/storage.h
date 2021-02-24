/**
 * @file storage.h
 * @author ljy
 * @date 2019-03-01
 * 
 *  储存控制
 */

#pragma once

#include "stdint.h"
#include "raw_flash.h"

#define stargeBINDKEYMAXLEN     80

typedef struct {
    uint8_t     bindflag;
    uint8_t     bindkeylen;
    uint8_t     bindkey[stargeBINDKEYMAXLEN];
}storageBindkey_def;

extern storageBindkey_def storageBindkey;

extern wiced_bool_t StoreBindKey(uint8_t *key, uint8_t len);
extern uint16_t storage_read_key(uint8_t *data);
extern uint16_t storage_read_sn(uint8_t *data);

