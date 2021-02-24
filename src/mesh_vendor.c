/*
* Copyright 2018, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor 
 *  Corporation. All rights reserved. This software, including source code, documentation and  related 
 * materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its 
 *  subsidiaries ("Cypress") and is protected by and subject to worldwide patent protection  
 * (United States and foreign), United States copyright laws and international treaty provisions. 
 * Therefore, you may use this Software only as provided in the license agreement accompanying the 
 * software package from which you obtained this Software ("EULA"). If no EULA applies, Cypress 
 * hereby grants you a personal, nonexclusive, non-transferable license to  copy, modify, and 
 * compile the Software source code solely for use in connection with Cypress's  integrated circuit 
 * products. Any reproduction, modification, translation, compilation,  or representation of this 
 * Software except as specified above is prohibited without the express written permission of 
 * Cypress. Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO  WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING,  BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to 
 * the Software without notice. Cypress does not assume any liability arising out of the application 
 * or use of the Software or any product or circuit  described in the Software. Cypress does 
 * not authorize its products for use in any products where a malfunction or failure of the 
 * Cypress product may reasonably be expected to result  in significant property damage, injury 
 * or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the 
 *  manufacturer of such system or application assumes  all risk of such use and in doing so agrees 
 * to indemnify Cypress against all liability.
*/

/** @file
 *
 *
 * This file shows how to create a device supporting a vendor specific model.
 */
#include "spar_utils.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_bt_mesh_core.h"
#include "wiced_bt_trace.h"
#include "wiced_timer.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_bt_mesh_core.h"

#include "wiced_bt_mesh_core_extra.h"

#ifdef HCI_CONTROL
#include "wiced_transport.h"
#include "hci_control_api.h"
#endif
#include "mesh_vendor.h"

#include "wiced_bt_cfg.h"

#include "wiced_bt_mesh_lpn.h"
#include "adv_pack.h"
#include "storage.h"
#include "includes.h"

/******************************************************
 *          Micro Prototypes
 ******************************************************/
#define TAG  "mesh_vendor"
#define WICED_LOG_LEVEL   WICDE_DEBUG_LEVEL
#define VENDOR_RETRY_TIMELENTH                     300              //单位ms
//#define LOG_VERBOSE(...) WICED_LOG_ERROR(__VA_ARGS__)
/******************************************************
 *          Function Prototypes
 ******************************************************/
void mesh_vendor_server_process_data(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
void mesh_vendor_server_process_get(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
void mesh_vendor_server_send_status(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
void release_event(wiced_bt_mesh_event_t *p_event);

typedef void (*VendorActionGet)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
typedef void (*VendorActionSet)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
typedef void (*VendorActionDelta)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
typedef void (*VendorActionToggle)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
typedef void (*VendorActionStart)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);
typedef void (*VendorActionStatus)(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len);

typedef struct resource_t
{
    uint8_t ResourceId;
    VendorActionGet Get;
    VendorActionSet Set;
    VendorActionDelta Delta;
    VendorActionToggle Toggle;
    VendorActionStart Start;
    VendorActionStatus Status;
    VendorActionStatus Flash;
} Resource;

typedef union{
    struct{
        uint8_t auth;
        uint8_t action;
        uint8_t seq;
        uint8_t resource;
        uint8_t pdu[16];
    }Item;
    uint8_t array[20];
}vendor_pdu_t;

typedef struct
{
    vendor_pdu_t vendor_pdu;                       //发送缓存
    uint8_t vendor_send_count;              //发送计数，非0时同时表示定时器正在运行
    uint8_t vendor_send_length;             //发送数据长度
    wiced_bool_t  vendor_timer_inited;      //发送定时器是否初始化
    wiced_timer_t vendor_timer;             //发送定时器
} mesh_vendor_send_t;

/******************************************************
 *          Variables Definitions
 ******************************************************/
 
static PLACE_DATA_IN_RETENTION_RAM uint8_t   vendor_send_seq = 0;

static mesh_vendor_send_t   vendor_send_array={
   .vendor_send_count = 0,
   .vendor_timer_inited = FALSE,
};
/******************************************************
 *               Function Definitions
 ******************************************************/

void obfs(uint8_t *data, uint16_t len)
{
    uint32_t seed = data[1];

    for (int i = 2; i < len; i++)
    {
        // WICED_BT_TRACE("a %x\n", seed);
        seed = 214013 * seed + 2531011;
        // WICED_BT_TRACE("b %x\n", seed);
        data[i] ^= (seed >> 16) & 0xff;
    }
}

uint8_t Auth(uint8_t *data, uint16_t len)
{
    uint8_t result = data[0];

    for (int i = 1; i < len; i++)
    {
        result ^= data[i];
    }

    return result;
}

int32_t transt_to_period(uint8_t t)
{
    int32_t value = t & 0x3f;

    switch (t & 0xc0)
    {
    case 0x00:
        return value * 10;
    case 0x40:
        return value * 100;
    case 0x80:
        return value * 1000;
    case 0xc0:
        return value * 60000;
    }
}

/*
 * This function is called when core receives a valid message for the define Vendor
 * Model (MESH_VENDOR_COMPANY_ID/MESH_VENDOR_MODEL_ID) combination.  The function shall return TRUE if it
 * was able to process the message, and FALSE if the message is unknown.  In the latter case the core
 * will call other registered models.
 */
wiced_bool_t mesh_vendor_message_handler(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    WICED_BT_TRACE("opcode:0x%04x model_id:%04x src:0x%04x dst:0x%04x\n", p_event->opcode, p_event->model_id, p_event->src, p_event->dst);
  
    // 0xffff model_id means request to check if that opcode belongs to that model
    if (p_event->model_id == 0xffff)
    {
        switch (p_event->opcode)
        {
        case MESH_VENDOR_OPCODE_GET:
        case MESH_VENDOR_OPCODE_SET:
        case MESH_VENDOR_OPCODE_SET_UNACKED:
        case MESH_VENDOR_OPCODE_STATUS:
            break;
        default:
            return WICED_FALSE;
        }
        return WICED_TRUE;
    }

    switch(p_event->opcode)
    {
    // this is a special case.  The provisioner configured this device to periodically publish
    // information.  Process is the same way as the GET command
    case WICED_BT_MESH_OPCODE_UNKNOWN:
    case MESH_VENDOR_OPCODE_GET:
        mesh_vendor_server_process_data(p_event, p_data, data_len);
        break;

    case MESH_VENDOR_OPCODE_SET:
        p_event->reply = WICED_FALSE;
        mesh_vendor_server_process_data(p_event, p_data, data_len);
        break;

    case MESH_VENDOR_OPCODE_SET_UNACKED:
        p_event->reply = WICED_FALSE;
        mesh_vendor_server_process_data(p_event, p_data, data_len);
        break;

    case MESH_VENDOR_OPCODE_STATUS:
        p_event->reply = WICED_FALSE;
        mesh_vendor_server_process_data(p_event, p_data, data_len);
        break;

    default:
        wiced_bt_mesh_release_event(p_event);
        return WICED_FALSE;
    }
    return WICED_TRUE;
}

void mesh_vendor_server_send_data(wiced_bt_mesh_event_t *p_event, uint16_t opcode, uint8_t *p_data, uint16_t data_len)
{
    p_event->opcode = opcode;
    obfs(p_data, data_len);
    p_data[0] = Auth(p_data, data_len);
    p_event->retrans_cnt  = 0;
    // for (int i = 0; i < data_len; i++)
    // {
    //     WICED_BT_TRACE("%02x ", p_data[i]);
    // }
    // WICED_BT_TRACE("\n");
    wiced_bt_mesh_core_send(p_event, p_data, data_len, release_event);
}

static uint32_t ACC_SN1[]=
{
	//0xAAAAAAAA,0xAAAAAAAA,
	//0xAAAAAAAA,0xAAAAAAAA	
	//74657374706430303030303030303031   V2.0
	0x74736574,0x30306470,
	0x30303030,0x31303030
	//31333031313730303030303030303031   V2.1
	//0x31303331,0x30303731,
	//0x30303030,0x31303030
};

static uint32_t ACC_KEY1[]=
{
	//0xAAAAAAAA,0xAAAAAAAA,
	//0xAAAAAAAA,0xAAAAAAAA
	//b9725b09b8593b311beaf62d9d499045   V2.0
	0x095b72b9,0x313b59b8,
	0x2df6ea1b,0x4590499d
	//b293d77403f108ad7be0822d2c4d2442   V2.1
	//0x74d793b2,0xad08f103,
	//0x2d82e07b,0x42244d2c
};
static void GetTokenInfo(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    uint8_t reply[36];
    uint16_t len=0;
    uint32_t lightontime;
    wiced_bt_mesh_event_t *p_event_rep;

    reply[0] = 0;
    reply[1] = p_data[1];
    reply[2] = p_data[2]|0x80;
    reply[3] = p_data[3];
    //TODO 从外置FLASH中读取存储的Token信息
    len = storage_read_sn( reply+4 );
    // len += storage_read_key( reply+4+len );
    WICED_LOG_DEBUG("sn&key:  ");
    for(uint8_t i=0;i<len;i++)
    {
        WICED_BT_TRACE("%02x ",reply[i+4]);
    }
    WICED_BT_TRACE("\n");
    obfs(reply+3,len+1);
    p_event_rep = wiced_bt_mesh_create_event(MESH_VENDOR_ELEMENT_INDEX, MESH_VENDOR_COMPANY_ID, MESH_VENDOR_MODEL_ID, p_event->src, p_event->app_key_idx);
    if (p_event == NULL)
    {
        WICED_LOG_DEBUG("create event error\n");
    }
    else
    {
        WICED_LOG_DEBUG("GetTokenInfo \n");
        for(uint8_t i=0;i<32;i++)
        {
            WICED_BT_TRACE("%02x ",reply[i+4]);
        }
        WICED_BT_TRACE("\n");
        mesh_vendor_server_send_data(p_event_rep, MESH_VENDOR_OPCODE_STATUS, reply, len+4);
    }
}

static void OnOffFlash(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    extern void LightFlash(uint16_t cycle, uint16_t times);

    int16_t flash_timers = 3;
    int16_t flash_cycle = 60;
    uint8_t reply[6];
    wiced_bt_mesh_event_t *p_event_rep;
    // for (int i = 0; i < data_len; i++)
    // {
    //     WICED_BT_TRACE("%02x ", p_data[i]);
    // }
    // WICED_BT_TRACE("\n");

    if (data_len > 4)
    {
        flash_cycle = p_data[4];
        if((flash_cycle > 0xBF) || (0==flash_cycle))
        {
            flash_cycle = 60;
        }
        else
        {
            flash_cycle = transt_to_period(flash_cycle);
        }
    }
    if (data_len > 5)
    {
        flash_timers = p_data[5];
    }
    LightFlash(flash_cycle, flash_timers);
    p_event_rep = wiced_bt_mesh_create_event(MESH_VENDOR_ELEMENT_INDEX, MESH_VENDOR_COMPANY_ID, MESH_VENDOR_MODEL_ID, p_event->src, p_event->app_key_idx);
    if (p_event == NULL)
    {
        WICED_LOG_DEBUG("create event error\n");
    }
    else
    {
        reply[0] = 0;
        reply[1] = p_data[1];
        reply[2] = p_data[2]|0x80;
        reply[3] = p_data[3];
        reply[4] = 0;
        reply[5] = 0;
        WICED_LOG_DEBUG("OnOffFlash \n");
        
        mesh_vendor_server_send_data(p_event_rep, MESH_VENDOR_OPCODE_STATUS, reply, 6);
    }
}

const Resource resource_list[] = {
    
    {.ResourceId = 0x01,
     .Flash = OnOffFlash},

     {.ResourceId = 0x09,
     .Get = GetTokenInfo},
    
};

/*
 * Scene Store Handler.  If the model need to be a part of a scene, store the data in the provided buffer.
 */
uint16_t mesh_model_vendor_scene_store_handler(uint8_t element_idx, uint8_t *p_buffer, uint16_t buffer_len)
{
    // return number of the bytes stored in the buffer.
    return 0;
}

/*
 * Scene Store Handler.  If the model need to be a part of a scene, restore the data from the provided buffer.
 */
uint16_t mesh_model_vendor_scene_recall_handler(uint8_t element_idx, uint8_t *p_buffer, uint16_t buffer_len, uint32_t transition_time, uint32_t delay)
{
    // return number of the bytes read from the buffer.
    return 0;
}

Resource *findResource(uint8_t resource_id)
{
    for (int i = 0; i < _countof(resource_list); i++)
    {
        if (resource_list[i].ResourceId == resource_id)
        {
            return (Resource *)resource_list + i;
        }
    }

    return NULL;
}

void mesh_vendor_server_process_data(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    uint32_t seed;

#if defined HCI_CONTROL
    wiced_bt_mesh_hci_event_t *p_hci_event;
#endif
    
    // Because the same app publishes and subscribes the same model, it will receive messages that it
    //sent out.
    if (p_event->src == wiced_bt_mesh_core_get_local_addr())
    {
        wiced_bt_mesh_release_event(p_event);
        return;
    }    
    
    if (data_len < 4)
    {
        wiced_bt_mesh_release_event(p_event);
        return;
    }    
    if (Auth(p_data, data_len) != 0)
    {
        return;
        wiced_bt_mesh_release_event(p_event);
    }    

    WICED_LOG_DEBUG("vs process data from:%04x opcode:%04x, reply:%04x len:%d\n",
                     p_event->src, p_event->opcode, p_event->reply, data_len);
    seed = p_data[1];
    WICED_BT_TRACE("0x%02x ",p_data[1]);
    for(uint8_t i=2; i<data_len; i++)
    {
        if(((i%8) == 0) && (i!=0))
        {
            WICED_BT_TRACE("\n");
        }
        seed = 214013 * seed + 2531011;
        p_data[i] ^= (seed >> 16) & 0xff;
        WICED_BT_TRACE("0x%02x ",p_data[i]);
    }
    WICED_BT_TRACE("\n");
    uint8_t rid = p_data[3];
    Resource *item = findResource(rid);
    if (item)
    {
            switch (p_data[2])
            {
            case ACTION_GET:
                if (item->Get)
                {
                    LOG_VERBOSE("call get\n");
                    item->Get(p_event, p_data, data_len);
                }
                break;
            case ACTION_SET:
                if (item->Set)
                {
                    LOG_VERBOSE("call set\n");
                    item->Set(p_event, p_data, data_len);
                }
                break;
            case ACTION_DELTA:
                if (item->Delta)
                {
                    LOG_VERBOSE("call delta\n");
                    item->Delta(p_event, p_data, data_len);
                }
                break;
            case ACTION_TOGGLE:
                if (item->Toggle)
                {
                    LOG_VERBOSE("call toggle\n");
                    item->Toggle(p_event, p_data, data_len);
                }
                break;
            case ACTION_START:
                if (item->Start)
                {
                    LOG_VERBOSE("call start\n");
                    item->Start(p_event, p_data, data_len);
                }
                break;
            case ACTION_STATUS:
                if (item->Status)
                {
                    LOG_VERBOSE("call status\n");
                    item->Status(p_event, p_data, data_len);
                }
                break;
            }
        }
#if defined HCI_CONTROL
    if ((p_hci_event = wiced_bt_mesh_create_hci_event(p_event)) != NULL)
        mesh_vendor_hci_event_send_data(p_hci_event, p_data, data_len);
#endif
    if (p_event->reply)
    {
        // return the data that we received in the command.  Real app can send anything it wants.
        mesh_vendor_server_send_status(wiced_bt_mesh_create_reply_event(p_event), p_data, data_len);
    }
    else
    {
        wiced_bt_mesh_release_event(p_event);
    }
}

void mesh_vendor_server_process_get(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    //mesh_vendor_server_send_status(wiced_bt_mesh_create_reply_event(p_event), (uint8_t *)"Hello", 6);
}

void release_event(wiced_bt_mesh_event_t *p_event)
{
    wiced_bt_mesh_release_event(p_event);
}

/*
 * Send Vendor Data status message to the Client
 */
void mesh_vendor_server_send_status(wiced_bt_mesh_event_t *p_event, uint8_t *p_data, uint16_t data_len)
{
    p_event->opcode = MESH_VENDOR_OPCODE_SET_UNACKED;
    p_event->retrans_cnt  = 0;
    
    //p_event->reply = 0;
    wiced_bt_mesh_core_send(p_event, p_data, data_len, release_event);
}

#ifdef HCI_CONTROL
/*
 * Send Vendor Data received from the mesh over transport
 */
void mesh_vendor_hci_event_send_data(wiced_bt_mesh_hci_event_t *p_hci_event, uint8_t *p_data, uint16_t data_len)
{
    uint8_t *p = p_hci_event->data;

    ARRAY_TO_STREAM(p, p_data, data_len);

    mesh_transport_send_data(HCI_CONTROL_MESH_EVENT_VENDOR_SET, (uint8_t *)p_hci_event, (uint16_t)(p - (uint8_t *)p_hci_event));
}

#endif

static void vendor_send_timer_callback(uint32_t arg)
{
    vendor_send_array.vendor_send_count--;
    if(0 == vendor_send_array.vendor_send_count)
    {
        wiced_stop_timer(&vendor_send_array.vendor_timer);
        return;
    }

    wiced_bt_mesh_event_t *p_event;
    uint8_t   element_idx = 0;
    uint16_t  app_key_idx = 0;
    uint16_t  dst         = 0;
    p_event = wiced_bt_mesh_create_event(element_idx, MESH_VENDOR_COMPANY_ID, MESH_VENDOR_MODEL_ID, dst, app_key_idx);
    if (p_event == NULL)
    {
        if(0 != vendor_send_array.vendor_send_count)
        {
            wiced_stop_timer(&vendor_send_array.vendor_timer);
        }
        return;
    }
    mesh_vendor_server_send_status(p_event, vendor_send_array.vendor_pdu.array, vendor_send_array.vendor_send_length );
}

void mesh_vendor_send_cmd(uint16_t user_cmd,uint8_t *pack_load,uint8_t len)
{
    uint32_t  length;
    uint8_t   element_idx = 0;
    uint16_t  app_key_idx = 0;
    uint16_t  dst         = 0;
    uint32_t  seed        = 0;
    //uint8_t   data[31];
    uint8_t*  p_data = vendor_send_array.vendor_pdu.array+1;
    wiced_bt_mesh_event_t *p_event;

    //识别是否已建立网络
    p_event = wiced_bt_mesh_create_event(element_idx, MESH_VENDOR_COMPANY_ID, MESH_VENDOR_MODEL_ID, dst, app_key_idx);
    if (p_event == NULL)
    {
        WICED_LOG_DEBUG("%s: Failed to create mesh event\n", __func__);
        //TODO 网络未建立，使用广播发送数据
        adv_vendor_send_cmd(user_cmd,pack_load,len,1);
        return;
    }
    if(WICED_FALSE == wiced_is_timer_in_use(&vendor_send_array.vendor_timer))  //初始化定时器
    {
        wiced_init_timer(&vendor_send_array.vendor_timer, &vendor_send_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
        vendor_send_array.vendor_timer_inited = WICED_TRUE;
    }
    if(0 != vendor_send_array.vendor_send_count)  //有数据正在发送中,关闭定时器
    {
        wiced_stop_timer(&vendor_send_array.vendor_timer);
    }
    
    vendor_send_seq++;
    length = 1;
    UINT8_TO_STREAM( p_data, vendor_send_seq);
    length += 1;
    UINT8_TO_STREAM( p_data, (uint8_t)((user_cmd>>8)&0xFF));       // Action
    length += 1;
    UINT8_TO_STREAM( p_data, (uint8_t)(user_cmd));       // resource
    length += 1;
    //装载数据
    for(uint8_t i=0; i<len; i++)
    {
        UINT8_TO_STREAM( p_data, *pack_load++);       // pdu
        length += 1;
    }
    p_event->retrans_time = 3;   //150ms  单位为50ms
    p_event->retrans_cnt  = 0;
    WICED_LOG_DEBUG("src=%x dst=%x\n",p_event->src,p_event->dst);
    WICED_LOG_DEBUG("%s: app_key_idx=%d, retrans_cnt=%d, retrans_time=%d ms,reply=%d\n", __func__,p_event->app_key_idx,p_event->retrans_cnt,p_event->retrans_time*50,p_event->reply);
    WICED_LOG_DEBUG("%s: send data:", __func__);
    seed = vendor_send_array.vendor_pdu.array[1];
    WICED_BT_TRACE("0x%02x ",vendor_send_array.vendor_pdu.array[1]);
    for(uint8_t i=2; i<length; i++)
    {
        if(((i%8) == 0) && (i!=0))
        {
            WICED_BT_TRACE("\n");
        }
        WICED_BT_TRACE("0x%02x ",vendor_send_array.vendor_pdu.array[i]);
        seed = 214013 * seed + 2531011;
        vendor_send_array.vendor_pdu.array[i] ^= (seed >> 16) & 0xff;
    }

    vendor_send_array.vendor_pdu.array[0] = 0;
    for(uint8_t i=1; i<length; i++)
    {
        vendor_send_array.vendor_pdu.array[0] ^= vendor_send_array.vendor_pdu.array[i];
    }

    WICED_BT_TRACE("\n");
    WICED_BT_TRACE("Auth = 0x%02x ",vendor_send_array.vendor_pdu.array[0]);
    WICED_BT_TRACE("\n");

    wiced_start_timer(&vendor_send_array.vendor_timer,VENDOR_RETRY_TIMELENTH);
    vendor_send_array.vendor_send_count = 1;
    vendor_send_array.vendor_send_length = length;
    mesh_vendor_server_send_status(p_event, vendor_send_array.vendor_pdu.array, length);
}
