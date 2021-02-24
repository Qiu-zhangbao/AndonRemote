//******************************************************************************
//*
//* 文 件 名 : 
//* 文件描述 :        
//* 作    者 : 张威/Andon Health CO.Ltd
//* 版    本 : V0.0
//* 日    期 : 
//*
//* 更新历史 : 
//*     日期       作者    版本     描述
//*         
//*          
//******************************************************************************

///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************

#include "wiced_bt_dev.h"
#include "wiced_bt_mesh_provision.h"
#include "wiced_bt_mesh_core_extra.h"
#include "wiced_bt_mesh_lpn.h"
#include "wiced_bt_mesh_models.h"
#include "mesh_vendor.h"
#include "Andon_App.h"
#include "wiced_bt_mesh_self_config.h"
#include "app_config.h"
#include "wiced_bt_mesh_app.h"
#include "includes.h"



///*****************************************************************************
///*                         宏定义区
///*****************************************************************************


///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
extern wiced_bool_t mesh_vendor_message_handler(uint16_t event, wiced_bt_mesh_event_t *, uint8_t *, uint16_t );
extern uint16_t mesh_model_vendor_scene_store_handler(uint8_t , uint8_t *, uint16_t );
extern uint16_t mesh_model_vendor_scene_recall_handler(uint8_t , uint8_t *, uint16_t , uint32_t , uint32_t );
static uint32_t mesh_app_proc_rx_cmd(uint16_t opcode, uint8_t *p_data, uint32_t length);

///*****************************************************************************
///*                         常量定义区
///*****************************************************************************
uint8_t mesh_mfr_name[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MANUFACTURER_NAME] = MANUFACTURER_CODE;
uint8_t mesh_model_num[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MODEL_NUMBER]     = PRODUCT_CODE;
uint8_t mesh_system_id[WICED_BT_MESH_PROPERTY_LEN_DEVICE_SERIAL_NUMBER]    = { 0 };
uint8_t mesh_fW_ver[WICED_BT_MESH_PROPERTY_LEN_DEVICE_FIRMWARE_REVISION]   = { 0 };
uint8_t mesh_hW_ver[WICED_BT_MESH_PROPERTY_LEN_DEVICE_HARDWARE_REVISION]   = { 0 };

wiced_bt_mesh_core_config_model_t   mesh_element1_models[] =
{
        WICED_BT_MESH_DEVICE,
#if USE_REMOTE_PROVISION 
    WICED_BT_MESH_MODEL_REMOTE_PROVISION_SERVER,
#endif
#if MESH_DFU
        WICED_BT_MESH_MODEL_FW_DISTRIBUTOR_UPDATE_SERVER,
#endif
        { MESH_COMPANY_ID_CYPRESS, WICED_BT_MESH_MODEL_ID_SELF_CONFIG, wiced_bt_mesh_self_config_message_handler, NULL, NULL},
        { MESH_VENDOR_COMPANY_ID, MESH_VENDOR_MODEL_ID, mesh_vendor_message_handler, mesh_model_vendor_scene_store_handler, mesh_model_vendor_scene_recall_handler },

};

// {
//         WICED_BT_MESH_DEVICE,
// #ifdef MESH_DFU
//         WICED_BT_MESH_MODEL_FW_DISTRIBUTION_SERVER,
//         WICED_BT_MESH_MODEL_FW_UPDATE_SERVER,
// #endif

// #if USE_REMOTE_PROVISION  
//         WICED_BT_MESH_MODEL_REMOTE_PROVISION_SERVER,
//         WICED_BT_MESH_MODEL_REMOTE_PROVISION_CLIENT,
// #endif

//         //WICED_BT_MESH_MODEL_ONOFF_CLIENT,
//         //WICED_BT_MESH_MODEL_LIGHT_LIGHTNESS_CLIENT,
//         //WICED_BT_MESH_MODEL_LEVEL_CLIENT,
//         { MESH_COMPANY_ID_CYPRESS, MESH_VENDOR_MODEL_ID, mesh_vendor_message_handler, mesh_model_vendor_scene_store_handler, mesh_model_vendor_scene_recall_handler },

        
// };

wiced_bt_mesh_core_config_element_t mesh_elements[] =
{
    {
        .location = MESH_ELEM_LOC_MAIN,                                 // location description as defined in the GATT Bluetooth Namespace Descriptors section of the Bluetooth SIG Assigned Numbers
        .default_transition_time = MESH_DEFAULT_TRANSITION_TIME_IN_MS,  // Default transition time for models of the element in milliseconds
        .onpowerup_state = WICED_BT_MESH_ON_POWER_UP_STATE_RESTORE,     // Default element behavior on power up
        .default_level = 0,                                             // Default value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_min = 1,                                                 // Minimum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_max = 0xffff,                                            // Maximum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .move_rollover = 0,                                             // If true when level gets to range_max during move operation, it switches to min, otherwise move stops.
        .properties_num = 0,                                            // Number of properties in the array models
        .properties = NULL,                                             // Array of properties in the element.
        .sensors_num = 0,                                               // Number of sensors in the sensor array
        .sensors = NULL,                                                // Array of sensors of that element
        .models_num = sizeof(mesh_element1_models) / sizeof(wiced_bt_mesh_core_config_model_t),                              // Number of models in the array models
        .models = mesh_element1_models,                                 // Array of models located in that element. Model data is defined by structure wiced_bt_mesh_core_config_model_t
    },
};
wiced_bt_mesh_core_config_t  mesh_config =
{
    .company_id         = MESH_VENDOR_COMPANY_ID,                   // Company identifier assigned by the Bluetooth SIG
    .product_id         = MESH_PID,                                 // Vendor-assigned product identifier
    .vendor_id          = MESH_VID,                                 // Vendor-assigned product version identifier
    //.firmware_id        = MESH_FWID,                                // Vendor-assigned firmware version identifier
    .replay_cache_size  = MESH_CACHE_REPLAY_SIZE,                   // Number of replay protection entries, i.e. maximum number of mesh devices that can send application messages to this device.
#if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
    .features           = WICED_BT_MESH_CORE_FEATURE_BIT_LOW_POWER, // A bit field indicating the device features. In Low Power mode no Relay, no Proxy and no Friend
    .friend_cfg         =                                           // Empty Configuration of the Friend Feature
    {
        .receive_window = 0,                                        // Receive Window value in milliseconds supported by the Friend node.
        .cache_buf_len  = 0,                                        // Length of the buffer for the cache
        .max_lpn_num    = 0                                         // Max number of Low Power Nodes with established friendship. Must be > 0 if Friend feature is supported. 
    },
    .low_power          =                                           // Configuration of the Low Power Feature
    {
        .rssi_factor           = 2,                                 // contribution of the RSSI measured by the Friend node used in Friend Offer Delay calculations.
        .receive_window_factor = 2,                                 // contribution of the supported Receive Window used in Friend Offer Delay calculations.
        .min_cache_size_log    = 3,                                 // minimum number of messages that the Friend node can store in its Friend Cache.
        .receive_delay         = 100,                               // Receive delay in 1 ms units to be requested by the Low Power node.
        .poll_timeout          = 42000,                              // Poll timeout in 100ms units to be requested by the Low Power node.
        .startup_unprovisioned_adv_to = 4                           // Interval in seconds of the advertisments of unprovisioned beacon and service on startup. Value 0 means default 30 seconds.
    },
#elif LOW_POWER_NODE == MESH_LOW_POWER_NODE_FRIEND
    .features = WICED_BT_MESH_CORE_FEATURE_BIT_FRIEND | WICED_BT_MESH_CORE_FEATURE_BIT_RELAY | WICED_BT_MESH_CORE_FEATURE_BIT_GATT_PROXY_SERVER,   // Supports Friend, Relay and GATT Proxy
    .friend_cfg         =                                           // Configuration of the Friend Feature(Receive Window in Ms, messages cache)
    {
        .receive_window        = 200,
        .cache_buf_len         = 300,                               // Length of the buffer for the cache
        .max_lpn_num           = 4                                  // Max number of Low Power Nodes with established friendship. Must be > 0 if Friend feature is supported. 
    },
    .low_power          =                                           // Configuration of the Low Power Feature
    {
        .rssi_factor           = 0,                                 // contribution of the RSSI measured by the Friend node used in Friend Offer Delay calculations.
        .receive_window_factor = 0,                                 // contribution of the supported Receive Window used in Friend Offer Delay calculations.
        .min_cache_size_log    = 0,                                 // minimum number of messages that the Friend node can store in its Friend Cache.
        .receive_delay         = 0,                                 // Receive delay in 1 ms units to be requested by the Low Power node.
        .poll_timeout          = 0                                  // Poll timeout in 100ms units to be requested by the Low Power node.
    },
#else
    WICED_BT_MESH_CORE_FEATURE_BIT_RELAY | WICED_BT_MESH_CORE_FEATURE_BIT_GATT_PROXY_SERVER,                           // support relay
    .friend_cfg         =                                           // Empty Configuration of the Friend Feature
    {
        .receive_window        = 0,                                // Receive Window value in milliseconds supported by the Friend node.
        .cache_buf_len         = 0,                                 // Length of the buffer for the cache
        .max_lpn_num           = 0                                  // Max number of Low Power Nodes with established friendship. Must be > 0 if Friend feature is supported. 
    },
    .low_power          =                                           // Configuration of the Low Power Feature
    {
        .rssi_factor           = 0,                                 // contribution of the RSSI measured by the Friend node used in Friend Offer Delay calculations.
        .receive_window_factor = 0,                                 // contribution of the supported Receive Window used in Friend Offer Delay calculations.
        .min_cache_size_log    = 0,                                 // minimum number of messages that the Friend node can store in its Friend Cache.
        .receive_delay         = 0,                                 // Receive delay in 1 ms units to be requested by the Low Power node.
        .poll_timeout          = 0                                  // Poll timeout in 100ms units to be requested by the Low Power node.
    },
#endif
    .gatt_client_only          = WICED_FALSE,                       // Can connect to mesh over GATT or ADV
    .elements_num  = (uint8_t)(sizeof(mesh_elements) / sizeof(mesh_elements[0])),   // number of elements on this device
    .elements      = mesh_elements                                  // Array of elements for this device
};

/*
 * Mesh application library will call into application functions if provided by the application.
 */
wiced_bt_mesh_app_func_table_t wiced_bt_mesh_app_func_table =
{
    Andon_App_Init,          // application initialization
    NULL,                   // Default SDK platform button processing
    NULL,                   // GATT connection status
    NULL,                   // attention processing
    NULL,                   // notify period set
    //mesh_app_proc_rx_cmd,   // WICED HCI command
    NULL,                    // WICED HCI command
#if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
    mesh_app_lpn_sleep,     // LPN sleep
#else
    NULL,                   // LPN sleep
#endif
    NULL                    // factory reset
};
///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数实现区
///*****************************************************************************    
/*
 * In 2 chip solutions MCU can send commands to change onoff state.
 */
static uint32_t mesh_app_proc_rx_cmd(uint16_t opcode, uint8_t *p_data, uint32_t length)
{
#ifdef HCI_CONTROL
    wiced_bt_mesh_event_t *p_event;

    switch (opcode)
    {
    case HCI_CONTROL_MESH_COMMAND_ONOFF_GET:
    case HCI_CONTROL_MESH_COMMAND_ONOFF_SET:
        break;

    default:
        return WICED_FALSE;
    }
    p_event = wiced_bt_mesh_create_event_from_wiced_hci(opcode, MESH_COMPANY_ID_BT_SIG, WICED_BT_MESH_CORE_MODEL_ID_GENERIC_ONOFF_CLNT, &p_data, &length);
    if (p_event == NULL)
    {
        WICED_LOG_DEBUG("bad hdr\n");
        return WICED_TRUE;
    }
    switch (opcode)
    {
    case HCI_CONTROL_MESH_COMMAND_ONOFF_GET:
        mesh_onoff_client_get(p_event, p_data, length);
        break;

    case HCI_CONTROL_MESH_COMMAND_ONOFF_SET:
        mesh_onoff_client_set(p_event, p_data, length);
        break;
    }
#endif
    return WICED_TRUE;
}