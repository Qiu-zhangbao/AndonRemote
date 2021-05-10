/*
 * Copyright 2018, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor
 * Corporation. All rights reserved. This software, including source code, documentation and
 * related materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its
 * subsidiaries ("Cypress") and is protected by and subject to worldwide patent protection 
 * (United States and foreign), United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software ("EULA"). If no EULA applies, Cypress
 * hereby grants you a personal, nonexclusive, non-transferable license to  copy, modify, and
 * compile the Software source code solely for use in connection with Cypress's  integrated circuit
 * products. Any reproduction, modification, translation, compilation,  or representation of this
 * Software except as specified above is prohibited without the express written permission of
 * Cypress. Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO  WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING,  BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to the
 * Software without notice. Cypress does not assume any liability arising out of the application
 * or use of the Software or any product or circuit  described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or failure of the
 * Cypress product may reasonably be expected to result  in significant property damage, injury
 * or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the
 * manufacturer of such system or application assumes  all risk of such use and in doing so agrees
 * to indemnify Cypress against all liability.
 */

/** @file
 *
 * This file shows how to handle device LPN + SDS sleep implementation
 */
#include "spar_utils.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_trace.h"
#include "wiced_hal_gpio.h"
#include "wiced_hal_wdog.h"
#include "wiced_timer.h"
#include "wiced_memory.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_hal_rand.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif

#include "adv_pack.h"
#include "wiced_bt_cfg.h"
#include "AES.h"
#include "raw_flash.h"
#include "includes.h"

#define TAG  "adv_pack"

#define ADV_RECE_FIFO_LEN      40
#define ADV_SEND_FIFO_LEN      40
#define ADV_PACK_LEN           28

// #define ADV_MANUADV_INDEX      10

#define adv_fifo_iptr_plus(a,b)     ((a+b)%ADV_RECE_FIFO_LEN)
#define adv_fifo_optr_plus(a,b)     ((a+b)%ADV_SEND_FIFO_LEN)
#define adv_send_seq_plus(a,b)      ((a+b)&0xFFFF)

#define ADV_CMD_RETRYTIMES     10

#define  AES_ENABLE               (1)

/******************************************************
 *          Constants
 ******************************************************/

/******************************************************
 *          Structures
 ******************************************************/
typedef union{
    struct
    {
        uint8_t adv_len;
        uint8_t adv_type;
        uint8_t companyid[2];
        uint8_t manu_name[3];
        uint8_t ttl;
        uint8_t crc_check[2];
        uint8_t seq;
        uint8_t segment;
        wiced_bt_device_address_t remote_mac;
        uint8_t remote_cmd[2];
        uint8_t cmd_load[8];
    }item;
    uint8_t adv_array[ADV_PACK_LEN];
}adv_rece_data_t;

typedef union{
    struct
    {
        uint8_t adv_len;
        uint8_t adv_type;
        uint8_t companyid[2];
        uint8_t manu_name[3];
        uint8_t ttl;
        uint8_t crc_check[2];
        uint8_t seq;
        uint8_t segment;
        wiced_bt_device_address_t local_mac;
        uint8_t local_cmd[2];
        uint8_t cmd_load[8];
        uint8_t count;
    }item;
    uint8_t adv_array[ADV_PACK_LEN+1];
}adv_send_data_t;


typedef struct{
    uint16_t cmd;
    void* cback;
}adv_receive_model_t;

/******************************************************
 *          Function Prototypes
 ******************************************************/
//extern void mesh_generate_random(uint8_t* random, uint8_t len);
extern void AndonPair_Cmdhandle(wiced_bt_device_address_t remote_mac, uint16_t cmd, void *p_adv_data,uint8_t len);
extern void tooling_handle(wiced_bt_ble_scan_results_t *p_scan_result,uint16_t cmd, uint8_t *data, uint8_t len);

static void adv_manuadv_send_handle( uint32_t params);
extern void AppGetMyMac(wiced_bt_device_address_t p_data);

/******************************************************
 *          Variables Definitions
 ******************************************************/
static adv_rece_data_t adv_fifo_in[ADV_RECE_FIFO_LEN] = {0};
static adv_send_data_t adv_fifo_out[ADV_SEND_FIFO_LEN] = {0};
static uint16_t   adv_fifo_in_iptr = 0;
static uint16_t   adv_fifo_out_optr = 0;
static uint16_t   adv_fifo_out_iptr = 0;
static wiced_bool_t   adv_send_enable = WICED_FALSE;
static uint8_t deviceUUID[16] = {0};
static PLACE_DATA_IN_RETENTION_RAM uint32_t   adv_send_seq = 0xFD;

adv_receive_model_t adv_receive_models[]=
{
    {ADV_CMD_PARI,AndonPair_Cmdhandle},
    {ADV_CMD_TOOLING, tooling_handle},
};

extern wiced_bt_ble_userscan_result_cback_t *adv_manufacturer_handler;

const uint8_t aes128key[16] = "com.jiuan.SLight";
#if RELEASE
const uint8_t adv_send_data_head[] = {ADV_PACK_LEN-1,BTM_BLE_ADVERT_TYPE_MANUFACTURER,0x04,0x08,'W','y','z'};
#else
const uint8_t adv_send_data_head[] = {ADV_PACK_LEN-1,BTM_BLE_ADVERT_TYPE_MANUFACTURER,0x04,0x08,'z','h','c'};
#endif

wiced_bt_ble_multi_adv_params_t  adv_pairadv_params =
{
    .adv_int_min = 80,            /**< Minimum adv interval */
    .adv_int_max = 96,            /**< Maximum adv interval */
    .adv_type = MULTI_ADVERT_NONCONNECTABLE_EVENT,               /**< Adv event type */
    .channel_map = BTM_BLE_DEFAULT_ADVERT_CHNL_MAP,            /**< Adv channel map */
    .adv_filter_policy = BTM_BLE_ADVERT_FILTER_WHITELIST_CONNECTION_REQ_WHITELIST_SCAN_REQ,      /**< Advertising filter policy */
    .adv_tx_power = MULTI_ADV_TX_POWER_MAX,           /**< Adv tx power */
    // .adv_tx_power = 8,             /**< Adv tx power */
    .peer_bd_addr = {0},           /**< Peer Device address */
    .peer_addr_type = BLE_ADDR_RANDOM,         /**< Peer LE Address type */
    .own_bd_addr = {0},            /**< Own LE address */
    .own_addr_type = BLE_ADDR_RANDOM         /**< Own LE Address type */
};

wiced_bt_ble_multi_adv_params_t adv_manudevadv_params =
 {
    .adv_int_min = 800,                                                                      /**< Minimum adv interval */
    .adv_int_max = 960,                                                                      /**< Maximum adv interval */
    .adv_type = MULTI_ADVERT_NONCONNECTABLE_EVENT,                                          /**< Adv event type */
    .channel_map = BTM_BLE_DEFAULT_ADVERT_CHNL_MAP,                                         /**< Adv channel map */
    .adv_filter_policy = BTM_BLE_ADVERT_FILTER_WHITELIST_CONNECTION_REQ_WHITELIST_SCAN_REQ, /**< Advertising filter policy */
    .adv_tx_power = MULTI_ADV_TX_POWER_MAX,                                                 /**< Adv tx power */
    // .adv_tx_power = 8,                                                 /**< Adv tx power */
    .peer_bd_addr = {0},                                                                    /**< Peer Device address */
    .peer_addr_type = BLE_ADDR_RANDOM,                                                      /**< Peer LE Address type */
    .own_bd_addr = {0},                                                                     /**< Own LE address */
    .own_addr_type = BLE_ADDR_RANDOM                                                        /**< Own LE Address type */
};


uint16_t remote_index = 0;
/******************************************************
 *               Function Definitions
 ******************************************************/

static void mesh_generate_random(uint8_t* random, uint8_t len)
{
    uint32_t r;
    uint8_t l;
    while (len)
    {
        r = wiced_hal_rand_gen_num();
        l = len > 4 ? 4 : len;
        memcpy(random, &r, l);
        len -= l;
        random += l;
    }
}

//*****************************************************************************
// 函数名称: CrcCalc
// 函数描述: 自定义广播的初始化--用于与灯的交互内容
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
uint16_t CrcCalc(uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint8_t j;
    union
    {
        uint16_t CRCX;
        uint8_t CRCY[2];
    } CRC;

    CRC.CRCX = 0xFFFF;
    for(i=0; i<length; i++)
    {
        CRC.CRCY[0] = (CRC.CRCY[0] ^ data[i]);
        for(j=0; j<8; j++)
        {
            if((CRC.CRCX & 0X0001) == 1)
                CRC.CRCX = (CRC.CRCX >> 1) ^ 0X1021;
            else
                CRC.CRCX >>= 1;
        }
    }
    return CRC.CRCX;
}


//*****************************************************************************
// 函数名称: adv_manuDevAdvStart
// 函数描述: 设置广播参数并启动广播---Wyze广播格式
// 函数输入:  *data：待广播数据  
//            len：数据长度
// 函数返回值: 
//*****************************************************************************/
void adv_manuDevAdvStart(uint8_t *data, uint8_t len)
{
    wiced_result_t wiced_result;

    wiced_start_multi_advertisements(MULTI_ADVERT_STOP, ADV_MANUDEVADV_INDEX);

    wiced_result = wiced_set_multi_advertisement_data(data, len, ADV_MANUDEVADV_INDEX);
    if (WICED_BT_SUCCESS != wiced_result)
    {
        return;
    }

    wiced_bt_dev_read_local_addr(adv_manudevadv_params.own_bd_addr);
    wiced_result = wiced_set_multi_advertisement_params(ADV_MANUDEVADV_INDEX, &adv_manudevadv_params);
    if (WICED_BT_SUCCESS != wiced_result)
    {
        return;
    }

    wiced_result = wiced_bt_notify_multi_advertisement_packet_transmissions(ADV_MANUDEVADV_INDEX, NULL, 8);
    if (TRUE != wiced_result)
    {
        return;
    }

    wiced_result = wiced_start_multi_advertisements(MULTI_ADVERT_START, ADV_MANUDEVADV_INDEX);
    if (WICED_BT_SUCCESS != wiced_result)
    {
        return;
    }
}

//*****************************************************************************
// 函数名称: adv_manuDevAdvStop
// 函数描述: 停止广播---Wyze广播格式
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void adv_manuDevAdvStop(void)
{
    // wiced_start_multi_advertisements(MULTI_ADVERT_STOP, ADV_MANUDEVADV_INDEX);
}

//*****************************************************************************
// 函数名称: adv_manuadv_send
// 函数描述: 设置广播参数并发送数据---自组网使用
// 函数输入:  *data：待广播数据  
//            len：数据长度
// 函数返回值: 
//*****************************************************************************/
void adv_manuadv_send(uint8_t *data,uint8_t len)
{
    wiced_result_t wiced_result;

    wiced_start_multi_advertisements(MULTI_ADVERT_STOP, ADV_PAIRADV_INDEX);

    wiced_result = wiced_set_multi_advertisement_data(data,len,ADV_PAIRADV_INDEX);
    if(WICED_BT_SUCCESS != wiced_result)
    {
        WICED_LOG_VERBOSE(" set_multi_advertisement_data failed : %d\r\n",wiced_result);
        return;
    }
    
    wiced_bt_dev_read_local_addr(adv_pairadv_params.own_bd_addr);
    wiced_result = wiced_set_multi_advertisement_params(ADV_PAIRADV_INDEX,&adv_pairadv_params);
    if(WICED_BT_SUCCESS != wiced_result)
    {
        WICED_LOG_VERBOSE(" set_multi_advertisement_params failed : %d\r\n",wiced_result);
        return;
    }

    wiced_result = wiced_bt_notify_multi_advertisement_packet_transmissions(ADV_PAIRADV_INDEX,adv_manuadv_send_handle,8);
    if(TRUE != wiced_result)
    {
        WICED_LOG_VERBOSE(" notify_multi_advertisement_packet_transmissions failed : %d\r\n",wiced_result);
        return;
    }

    wiced_result = wiced_start_multi_advertisements(MULTI_ADVERT_START,ADV_PAIRADV_INDEX);
    if(WICED_BT_SUCCESS != wiced_result)
    {
        WICED_LOG_VERBOSE(" start_multi_advertisements failed : %d\r\n",wiced_result);
        return;
    }
}

//*****************************************************************************
// 函数名称: adv_send_Scheduling
// 函数描述: 广播数据的调度处理，数据开始发送或发送完成后调用此函数确定是否将数据发送
// 函数输入:  params：广播数据发送状态
// 函数返回值: 
//*****************************************************************************/
static void adv_send_Scheduling(void)
{
    if(adv_fifo_out_iptr != adv_fifo_out_optr)
    {
        if(WICED_TRUE != adv_send_enable)
        {
            //WICED_LOG_VERBOSE(" adv_fifo_out_optr = %d  adv_fifo_out_iptr = %d",adv_fifo_out_optr,adv_fifo_out_iptr);
            adv_manuadv_send( adv_fifo_out[adv_fifo_out_optr].adv_array,ADV_PACK_LEN);
            adv_send_enable = WICED_TRUE;
        }
    }
    else
    {
        //还在发送过程中，说明发送缓存满了
        if(WICED_TRUE == adv_send_enable)
        {
            memset(adv_fifo_out[adv_fifo_out_optr].adv_array,0,sizeof(adv_send_data_t));
            adv_fifo_out_optr = adv_fifo_optr_plus(adv_fifo_out_optr, 1);
            adv_manuadv_send(adv_fifo_out[adv_fifo_out_optr].adv_array, ADV_PACK_LEN);
        }
        else //发送完成
        {
            wiced_start_multi_advertisements(MULTI_ADVERT_STOP, ADV_PAIRADV_INDEX);
            adv_send_enable = WICED_FALSE;
        }
    }
}

//*****************************************************************************
// 函数名称: adv_manuadv_send_handle
// 函数描述: 自定义广播数据发送完成的回调处理函数
// 函数输入:  params：广播数据发送状态
// 函数返回值: 
//*****************************************************************************/
static void adv_manuadv_send_handle( uint32_t params)
{
    //TODO !!!!!   防止SDK运行异常,意外抛出不使用这个广播的发送状态回调，
    //但是这么做是有风险的，手册里说的是此处返回是结果，并不会和ADV_PAIRADV_INDEX关联 实际测试时关联的  params= ADV_PAIRADV_INDEX*256 + 状态
    if(ADV_PAIRADV_INDEX != params/256)
    {
        return;
    }
    if(WICED_BT_ADV_NOTIFICATION_DONE == (params&0xFF))
    {
        //WICED_LOG_VERBOSE(" Send adv Handle: %d\r\n",params);
        //WICED_LOG_VERBOSE(" Send adv count: %d\r\n",adv_fifo_out[adv_fifo_out_optr].item.count);
        if(adv_fifo_out[adv_fifo_out_optr].item.count > 1)
        {
            adv_fifo_out[adv_fifo_out_optr].item.count--;
        }
        else
        {
            //仅在主动发送完成后将发送指针移动，防止SDK运行异常,意外抛出不使用这个广播的发送状态回调
            //这种防错机制比上面的要靠谱
            if(WICED_TRUE == adv_send_enable)
            {
                WICED_LOG_VERBOSE(" adv_fifo_out_optr = %d  adv_fifo_out_iptr = %d",adv_fifo_out_optr,adv_fifo_out_iptr);
                memset(adv_fifo_out[adv_fifo_out_optr].adv_array,0,sizeof(adv_send_data_t));
                adv_fifo_out_optr = adv_fifo_optr_plus(adv_fifo_out_optr,1);
                adv_send_enable = WICED_FALSE;
                adv_send_Scheduling();
            }
        }
    }
}

//*****************************************************************************
// 函数名称: adv_recevier_handle
// 函数描述: 接收广播数据处理
// 函数输入:  *p_scan_result：
//           *p_adv_data： 接收到数据包 
// 函数返回值:  WICED_FALSE：数据未被处理
//             WICED_TRUE： 数据已处理
//*****************************************************************************/
static wiced_bool_t adv_recevier_handle (wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data)
{
    adv_rece_data_t adv_data;
    uint32_t seed;
    wiced_bt_device_address_t own_bd_addr;
    uint16_t rece_cmd;
    uint16_t rece_crc;

    //数据头不匹配，不做解析
    //WICED_LOG_VERBOSE("Rece ADV MANUFACTURER: \r\n");
    if(0 != memcmp(p_adv_data,adv_send_data_head,sizeof(adv_send_data_head)))
    {
        //WICED_LOG_VERBOSE("Rece isn't own packload: \r\n");
        return WICED_FALSE;
    }
    for(uint16_t i=0; i<ADV_RECE_FIFO_LEN; i++)  //检验数据是否已存储在缓冲中
    {
        //数据比对时 从seq字节开始比对
        if(0 == memcmp(p_adv_data+sizeof(adv_send_data_head)+1,adv_fifo_in[i].item.crc_check,ADV_PACK_LEN-sizeof(adv_send_data_head)-1))
        {
            //WICED_LOG_VERBOSE("Rece old packload: \r\n");
            return WICED_TRUE;
        }
    }
    //WICED_LOG_VERBOSE("Rece ADV MANUFACTURER: \r\n");
    //copy data
    memcpy(adv_data.adv_array,p_adv_data,ADV_PACK_LEN);
    memcpy(adv_fifo_in[adv_fifo_in_iptr].adv_array,p_adv_data,ADV_PACK_LEN);
#if AES_ENABLE
    //decode
    AES_Decrypt(adv_data.item.remote_mac,adv_data.item.remote_mac, 16,aes128key);
#endif
    //校验
    rece_crc = adv_data.item.crc_check[0];
    rece_crc = rece_crc*256 + adv_data.item.crc_check[1];
    if(rece_crc !=  CrcCalc(adv_data.adv_array+sizeof(adv_send_data_head)+3,sizeof(adv_data.adv_array)-(sizeof(adv_send_data_head)+3)))
    {
        return WICED_TRUE;
    }
    //去混淆
    seed = adv_data.item.seq;
    for (int i = sizeof(adv_send_data_head)+4; i < sizeof(adv_data.adv_array); i++)
    {
        seed = 214013 * seed + 2531011;
        adv_data.adv_array[i] ^= (seed >> 16) & 0xff;
    }
    //wiced_bt_dev_read_local_addr(own_bd_addr);
    //receiver own message
    AppGetMyMac(own_bd_addr);
    if(0 == memcmp(adv_data.item.remote_mac,own_bd_addr,sizeof(wiced_bt_device_address_t)))
    {
        //adv_fifo_in_iptr = adv_fifo_iptr_plus(adv_fifo_in_iptr,1);
        return WICED_TRUE;
    }

    WICED_LOG_DEBUG("My MAC: %B, Rece Mac %B\n",own_bd_addr,adv_data.item.remote_mac);
    WICED_LOG_DEBUG("Rece ADV MANUFACTURER: ");
    for(uint16_t i=0; i<ADV_PACK_LEN; i++)
    {
        WICED_BT_TRACE("%02X",adv_data.adv_array[i]);
    }
    WICED_BT_TRACE("\r\n");

    adv_data.item.ttl = 0;  //遥控器不转发数据
    if(adv_data.item.ttl > 0)
    {
        //relay
        memcpy(adv_fifo_out[adv_fifo_out_iptr].adv_array,adv_fifo_in[adv_fifo_in_iptr].adv_array,ADV_PACK_LEN);
        adv_fifo_out[adv_fifo_out_iptr].item.ttl -= 1;
        adv_fifo_out[adv_fifo_out_iptr].item.count = ADV_CMD_RETRYTIMES;
        //adv_manuadv_send(adv_fifo_out[adv_fifo_optr].adv_array,ADV_PACK_LEN);
        adv_fifo_out_iptr = adv_fifo_optr_plus(adv_fifo_out_iptr,1);
        adv_send_Scheduling();
    }


    adv_fifo_in_iptr = adv_fifo_iptr_plus(adv_fifo_in_iptr,1);
    //TODO 临时处理，后续需区分指令类型，抛给应用逻辑处理
    rece_cmd = adv_data.item.remote_cmd[0];
    rece_cmd = rece_cmd*256 + adv_data.item.remote_cmd[1];
    for(uint8_t i=0; i<sizeof(adv_receive_models)/sizeof(adv_receive_model_t);i++)
    {
        // WICED_LOG_DEBUG("Rece cmd: 0x%04x\r\n",rece_cmd);
        if(adv_receive_models[i].cmd == rece_cmd)
        {
            wiced_ble_adv_cmd_cback_t cback;
            cback = adv_receive_models[i].cback;
            (*cback)(adv_data.item.remote_mac, rece_cmd, adv_data.item.cmd_load,sizeof(adv_data.item.cmd_load));
            return WICED_TRUE;
        }
    }
    return WICED_TRUE;
}

//*****************************************************************************
// 函数名称: adv_pack_init
// 函数描述: 自定义广播的初始化--用于与灯的交互内容
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void adv_pack_init(void)
{
    #include "wiced_hal_nvram.h"
    #include "mesh_application.h"

    extern uint32_t mesh_nvram_access(wiced_bool_t write, int inx, uint8_t* node_info, uint16_t len, wiced_result_t *p_result);

    wiced_result_t result;
    mesh_nvram_access(WICED_FALSE, NVRAM_ID_LOCAL_UUID, deviceUUID, 16, &result);
    
    adv_manufacturer_handler = adv_recevier_handle;
    adv_fifo_in_iptr = 0;
    adv_fifo_out_iptr = 0;
    adv_fifo_out_optr = 0;
    adv_send_enable = WICED_FALSE;
#if AES_ENABLE
    AES_Init(aes128key);
#endif // AES_ENABLE
    advpackReadRemoteIndex();
}

//*****************************************************************************
// 函数名称: adv_vendor_send_cmd
// 函数描述: 打包自定义广播数据并存储到发送序列中
// 函数输入: user_cmd: 命令
//          *pack_load: 命令参数  
//          len: 命令参数的长度
//          ttl: 数据包可被转发的次数
// 函数返回值: 
//*****************************************************************************/
void adv_vendor_send_cmd(uint16_t user_cmd,uint8_t *pack_load,uint8_t len,uint8_t ttl)
{
    uint16_t length = 0;
    uint32_t seed;
    uint16_t crc_send;
    wiced_bt_device_address_t own_bd_addr;
    uint8_t  mac_index=0;

    WICED_LOG_DEBUG("Send cmd: 0x%02x\r\n",user_cmd);
    memcpy(adv_fifo_out[adv_fifo_out_iptr].adv_array,adv_send_data_head,sizeof(adv_send_data_head));
    //adv_fifo_out[adv_fifo_out_iptr].item.seq[0] = (uint8_t)((adv_send_seq>>8)&0xFF);
    adv_fifo_out[adv_fifo_out_iptr].item.seq = (uint8_t)(adv_send_seq&0xFF);
    adv_fifo_out[adv_fifo_out_iptr].item.segment = 0;
    adv_send_seq = adv_send_seq_plus(adv_send_seq,1);
    adv_fifo_out[adv_fifo_out_iptr].item.ttl = ttl;
    //wiced_bt_dev_read_local_addr(own_bd_addr);
    // memcpy(adv_fifo_out[adv_fifo_out_iptr].item.local_mac, deviceUUID,sizeof(wiced_bt_device_address_t));
    AppGetMyMac(own_bd_addr);
    memcpy(adv_fifo_out[adv_fifo_out_iptr].item.local_mac, own_bd_addr,sizeof(wiced_bt_device_address_t));
    mac_index = *(uint8_t *)(adv_fifo_out[adv_fifo_out_iptr].item.local_mac);
    mac_index = (mac_index&0x3F) | (remote_index<<6);
    *(uint8_t *)(adv_fifo_out[adv_fifo_out_iptr].item.local_mac) = mac_index;
    adv_fifo_out[adv_fifo_out_iptr].item.local_cmd[0] = (uint8_t)((user_cmd>>8)&0xFF);
    adv_fifo_out[adv_fifo_out_iptr].item.local_cmd[1] = (uint8_t)(user_cmd);

    //TODO 暂不做分包处理
    if(len > sizeof(adv_fifo_out[adv_fifo_out_iptr].item.cmd_load))
        len = sizeof(adv_fifo_out[adv_fifo_out_iptr].item.cmd_load);
    memcpy(adv_fifo_out[adv_fifo_out_iptr].item.cmd_load,pack_load,len);

    mesh_generate_random(adv_fifo_out[adv_fifo_out_iptr].item.cmd_load+len,\
                         sizeof(adv_fifo_out[adv_fifo_out_iptr].item.cmd_load)-len);

    //数据混淆处理
    seed = adv_fifo_out[adv_fifo_out_iptr].item.seq;
    for (int i = sizeof(adv_send_data_head)+4; i < (sizeof(adv_fifo_out[adv_fifo_out_iptr].adv_array)-1); i++)
    {
        seed = 214013 * seed + 2531011;
        adv_fifo_out[adv_fifo_out_iptr].adv_array[i] ^= (seed >> 16) & 0xff;
    }
    //校验
    crc_send = CrcCalc(adv_fifo_out[adv_fifo_out_iptr].adv_array+(sizeof(adv_send_data_head)+3),
                       sizeof(adv_fifo_out[adv_fifo_out_iptr].adv_array)-(sizeof(adv_send_data_head)+4));
    adv_fifo_out[adv_fifo_out_iptr].item.crc_check[0] = (uint8_t)(crc_send>>8);
    adv_fifo_out[adv_fifo_out_iptr].item.crc_check[1] = (uint8_t)(crc_send);
#if AES_ENABLE
    //AES加密
    AES_Encrypt(adv_fifo_out[adv_fifo_out_iptr].item.local_mac,adv_fifo_out[adv_fifo_out_iptr].item.local_mac,16,aes128key);
#endif
    adv_fifo_out[adv_fifo_out_iptr].item.count = ADV_CMD_RETRYTIMES;
    adv_fifo_out_iptr = adv_fifo_optr_plus(adv_fifo_out_iptr,1);
    adv_send_Scheduling();
}

//*****************************************************************************
// 函数名称: advpackReWriteRemoteIndex
// 函数描述: 重写遥控器的Index计数，bit1 bit0有效，占用一个字节
//          此计数用于识别遥控器是否被重置(或解除绑定)，每次解除绑定自加1，
//          占用高位的
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void advpackReWriteRemoteIndex(void)
{
    remote_index++;
    //TODO 将此值写入到FLASH中
    flash_write_erase(FLASH_ADDR_CNT, (uint8_t*)(&remote_index), sizeof(remote_index),WICED_TRUE);

}
//*****************************************************************************
// 函数名称: advpackReadRemoteIndex
// 函数描述: 读取遥控器的Index计数，bit1 bit0有效，占用一个字节
//          此计数用于识别遥控器是否被重置(或解除绑定)，每次解除绑定自加1，
//          占用高位的
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void advpackReadRemoteIndex(void)
{
    if(sizeof(remote_index) != flash_app_read(FLASH_ADDR_CNT, (uint8_t*)(&remote_index), sizeof(remote_index)))
    {
        remote_index = 0;
    }
}

//*****************************************************************************
// 函数名称: advpackResetRemoteIndex
// 函数描述: 重置遥控器的Index计数，bit1 bit0有效，占用一个字节
//          此计数用于识别遥控器是否被重置(或解除绑定)，每次解除绑定自加1，
//          占用高位的
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void advpackResetRemoteIndex(void)
{
    remote_index == 0;
    //TODO 将此值写入到FLASH中
    flash_write_erase(FLASH_ADDR_CNT, (uint8_t*)(&remote_index), sizeof(remote_index),WICED_TRUE);
}


