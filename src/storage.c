/**
 * @file storage.c
 * @author ljy
 * @date 2019-03-03
 * 
 *  NVRAM存储
 * 
 */

#include "storage.h"
#include "wiced_bt_types.h"
#include "raw_flash.h"

#include "log.h"

storageBindkey_def storageBindkey={0};

wiced_bool_t StoreBindKey(uint8_t *key, uint8_t len)
{
    if(len > stargeBINDKEYMAXLEN)
    {
        return WICED_FALSE;
    } 
    memset(storageBindkey.bindkey,0,stargeBINDKEYMAXLEN);
    if(len == 0){
        storageBindkey.bindflag = WICED_FALSE;
    }else{
        memcpy(storageBindkey.bindkey,key,len);
        storageBindkey.bindflag = WICED_TRUE;
    }
    storageBindkey.bindkeylen = len;
    if (sizeof(storageBindkey) != flash_write_erase(FLASH_ADDR_BINDKEY, (uint8_t*)(&storageBindkey), sizeof(storageBindkey), WICED_TRUE))
    {
        storageBindkey.bindflag = WICED_FALSE;
        LOG_WARNING("store config failed.\n"); 
        return WICED_FALSE;
    } 
    return WICED_TRUE;
}

void StoreReadBindKey(void)
{
    memset((uint8_t*)(&storageBindkey),0,sizeof(storageBindkey));
    if(0 == flash_app_read_mem(FLASH_ADDR_BINDKEY,(uint8_t*)(&storageBindkey), sizeof(storageBindkey)))
    {
        memset((uint8_t*)(&storageBindkey),0,sizeof(storageBindkey));
        LOG_WARNING("StoreReadBindKey failed.\n");
        storageBindkey.bindflag = WICED_FALSE;
        storageBindkey.bindkeylen = 0;
    }
}

uint16_t storage_read_sn(uint8_t *data)
{
    LOG_DEBUG("sn addr:%08x\n",FLASH_ADDR_XXTEA_SN);
    if(0 == flash_app_read_mem(FLASH_ADDR_XXTEA_SN,data,FLASH_XXTEA_SN_LEN))
    {
        memset(data,0,FLASH_XXTEA_SN_LEN);
        LOG_WARNING("storage_read_sn failed.\n");
    }
    return FLASH_XXTEA_SN_LEN;
}

// uint16_t storage_read_key(uint8_t *data)
// {
//     if(0 == flash_app_read_mem(FLASH_ADDR_XXTEA_KEY,data,FLASH_ADDR_XXTEA_KEY_LEN))
//     {
//         memset(data,0,FLASH_ADDR_XXTEA_KEY_LEN);
//         LOG_WARNING("storage_read_key failed.\n");
//     }
//     return FLASH_ADDR_XXTEA_KEY_LEN;
// }

#ifdef ANDON_TEST

#ifndef DID_NEW_TYPE
static uint8_t ACC_SN1[] = "JA_SLA0_20031100002020031100000000";
#else
#if RELEASE
static uint8_t ACC_SN1[] = "JA_SLA0_7C78B20C0A15,3516967f16f2caf20c7de5772bf214a7";
#else
static uint8_t ACC_SN1[] = "JA_SLA0_7C78B20C0801,3d9417573eb286786460625c2cab539f";
// static uint8_t ACC_SN1[] = "JA_SLA0_7C78B20C0A18,2cb09042452537abf36cc9b7c4fe2143";
// static uint8_t ACC_SN1[] = "JA_SLA0_7C78B20C0A12,92954c9354f832bfd7d4d1c2f4076903";
// static uint8_t ACC_SN1[] = "JA_SLA0_7C78B20C0A10,9b98dfe6f7072c6d13b16d63e05d3a07";

#endif
// static uint8_t ACC_SN1[] = "CO_TH1_firmware_test,CO_TH1_firmware_test";
#endif

// {
// 	//0xAAAAAAAA,0xAAAAAAAA,
// 	//0xAAAAAAAA,0xAAAAAAAA	
// 	//74657374706430303030303030303031   V2.0
// 	0x74736574,0x30306470,
// 	0x30303030,0x31303030
// 	//31333031313730303030303030303031   V2.1
// 	//0x31303331,0x30303731,
// 	//0x30303030,0x31303030
// };

// static uint8_t ACC_KEY1[]="2020031100000000";
// {
// 	//0xAAAAAAAA,0xAAAAAAAA,
// 	//0xAAAAAAAA,0xAAAAAAAA
// 	//b9725b09b8593b311beaf62d9d499045   V2.0
// 	0x095b72b9,0x313b59b8,
// 	0x2df6ea1b,0x4590499d
// 	//b293d77403f108ad7be0822d2c4d2442   V2.1
// 	//0x74d793b2,0xad08f103,
// 	//0x2d82e07b,0x42244d2c
// };

void storage_save_keyandsn(void)
{
    if(0 == flash_write_erase(FLASH_ADDR_XXTEA_SN,ACC_SN1,FLASH_XXTEA_SN_LEN,WICED_TRUE))
    {
        LOG_WARNING("storage_save_key failed.\n");
    }
    // if(0 == flash_write_erase(FLASH_ADDR_XXTEA_SN,ACC_SN1,FLASH_XXTEA_SN_LEN,WICED_FALSE))
    // {
    //     LOG_WARNING("storage_save_sn failed.\n");
    // }
    return ;
}

void PrintfXxteakeyandsn(void)
{
    uint8_t data[FLASH_XXTEA_SN_LEN+1]={0};
    
    storage_read_sn(data);
    WICED_BT_TRACE_ARRAY(data,FLASH_XXTEA_SN_LEN,"\nRead Did: ");
    LOG_DEBUG("Read Did:%s\n", data);
}

#endif

