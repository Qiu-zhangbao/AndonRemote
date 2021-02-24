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
 *
 * This file set up the initial network data contents.
 */
#include "wiced.h"
#include "wiced_bt_mesh_core_extra.h"
#include "wiced_bt_mesh_lpn.h"

wiced_bt_mesh_app_key_init_t app_keys[3] =
{
    // Generic app_key
    {
        0,            // uint16_t app_key_pos
        0x01c3,       // uint16_t app_key_index
        // uint8_t  app_key[MESH_KEY_LEN]
        {0x09, 0x49, 0xFD, 0x10, 0x40, 0x8F, 0x67, 0x18, 0x14, 0xDF, 0x01, 0x0F, 0x69, 0x07, 0xE0, 0x5A}
    },

    // Setup app_key
    {
        1,            // uint16_t app_key_pos
        0x026a,       // uint16_t app_key_index
        // uint8_t  app_key[MESH_KEY_LEN]
        {0x77, 0x2C, 0x7A, 0x8E, 0xCB, 0xCA, 0xE9, 0xE9, 0x5C, 0xA6, 0xCB, 0xE8, 0xD4, 0x7F, 0xF1, 0x1D}
    },

    // Vendor app_key
    {
        2,            // uint16_t app_key_pos
        0x0ae6,       // uint16_t app_key_index
        // uint8_t  app_key[MESH_KEY_LEN]
        {0x02, 0x5D, 0x0B, 0x69, 0x75, 0x1F, 0x48, 0x76, 0x2E, 0xB1, 0x7E, 0x0F, 0x0C, 0x76, 0x86, 0x38}
    }
};


wiced_bt_mesh_node_init_t init_node = 
{
    0,                                // uint16_t node_id: Use random generated node ID
#if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
    NODE_STATE_PROXY_NOT_SUPPORTED,   // uint8_t state_gatt_proxy
    NODE_STATE_SEC_NET_BEACON_OFF,    // uint8_t state_sec_net_beacon
    NODE_STATE_RELAY_NOT_SUPPORTED,   // uint8_t state_gatt_proxy
#else
    NODE_STATE_PROXY_ON,              // uint8_t state_gatt_proxy
    NODE_STATE_SEC_NET_BEACON_ON,     // uint8_t state_sec_net_beacon
    NODE_STATE_RELAY_ON,              // uint8_t state_relay
#endif
#if LOW_POWER_NODE == MESH_LOW_POWER_NODE_FRIEND
    NODE_STATE_FRIEND_ON,
#else
    NODE_STATE_FRIEND_NOT_SUPPORTED,  // uint8_t state_friend
#endif
    0x6a,                             // uint8_t state_relay_retrans, 3 times, 100 ms interval
    0x3f,                             // uint8_t state_default_ttl
    0x4a,                             // uint8_t state_net_trans, repeat 2 times, 100 ms

    0,                                // net_key_pos
    // uint8_t  net_key[MESH_KEY_LEN];
    {0xD1, 0xF1, 0xF1, 0x96, 0xD2, 0x82, 0x17, 0x9F, 0x50, 0x5B, 0x1E, 0x91, 0xBC, 0xE1, 0x8C, 0x2E},

    WICED_TRUE,             // wiced_bool_t use_random_dev_key: use random device key
    // uint8_t  dev_key[MESH_KEY_LEN];
    {0xE3, 0xA2, 0xB4, 0x59, 0xDF, 0x28, 0x94, 0x52, 0x85, 0x7C, 0x9B, 0x36, 0x28, 0x10, 0x33, 0x7E},

    3,                      // app_key_num
    app_keys                // app_key
};
