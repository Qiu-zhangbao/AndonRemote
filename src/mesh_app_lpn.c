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
#include "wiced_bt_trace.h"
#include "wiced_hal_gpio.h"
#include "wiced_hal_wdog.h"
#include "wiced_timer.h"
#include "wiced_sleep.h"
#include "wiced_bt_mesh_core.h"
#include "hci_control_api.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif

#include "clock_timer.h"
#include "wiced_bt_mesh_lpn.h"
#include "Andon_App.h"
#include "includes.h"


#define WICED_SLEEP_MODE     WICED_SLEEP_MODE_NO_TRANSPORT
#define WICED_SLEEP_TYPE     WICED_SLEEP_TYPE_SDS

#define SDS_SLEEP_LENGTH     (70UL*60*1000*1000)       //1 hour

#define SDS_SLEEP_LENGTH_MIN (10 * 60 * 1000 * 1000) // 1 mins in usec
#define SDS_IDLE_DELAY       50   // Delay 10ms to SDS after entering LPN mode


#define TAG  "app lpn"

#define WICED_LOG_LEVEL   WICDE_DEBUG_LEVEL

/******************************************************
 *          Constants
 ******************************************************/

/******************************************************
 *          Structures
 ******************************************************/
typedef struct
{
    OSAPI_TIMER          sds_sleep_timer;
    wiced_timer_t        sds_idle_timer;
    wiced_sleep_config_t sds_sleep_config;
    uint32_t             sds_sleep_length;
    uint8_t              lpn_state;    // LPN state: IDLE or NOT_IDLE
    uint32_t             lpn_elapsed_time;
    UINT64               lpn_time;
} mesh_app_lpn_t;


/******************************************************
 *          Function Prototypes
 ******************************************************/
static void     mesh_app_lpn_sds_idle_timer_callback(uint32_t arg);
static void     mesh_app_lpn_sds_wake_timer_callback(INT32 arg, UINT32 overTimeInUs);
static uint32_t mesh_app_lpn_sleep_poll(wiced_sleep_poll_type_t type);

extern void mesh_app_hci_sleep(void);
extern wiced_bool_t mesh_app_gatt_is_connected(void);
extern void keyscan_stuckKey_timer_expired_callback(UINT32 args);

/******************************************************
 *          Variables Definitions
 ******************************************************/
extern uint8_t timedWake_SDS;
void (*Sleep_Config_Cb)(void) = NULL;

// Application state
mesh_app_lpn_t app_lpn_state;
static uint32_t lpn_idle_count;

PLACE_IN_ALWAYS_ON_RAM UINT64 app_lpn_sleep_time;

/******************************************************
 *               Function Definitions
 ******************************************************/
/*
 * Callback function from mesh core to notify the device to enter LPN sleep state
 */

// Johnson 2020.08.01
// ================================================
void set_low_power_node_starte_idle(uint8_t idle)
{
    app_lpn_state.lpn_state = idle;
}

uint32_t record_sleep_time;
void mesh_app_lpn_start_idle_timer(uint32_t duration)
{
    record_sleep_time = duration;
    WICED_BT_TRACE("MESH_LOW_POWER_NODE_STATE_NOT_IDLE\n");
    app_lpn_state.lpn_state = MESH_LOW_POWER_NODE_STATE_NOT_IDLE;
    // Start the SDS idle wait timer
    wiced_result_t result;    
    if(wiced_is_timer_in_use(&app_lpn_state.sds_idle_timer))
    {
        wiced_stop_timer(&app_lpn_state.sds_idle_timer);
    }    
    else
    {
        wiced_init_timer(&app_lpn_state.sds_idle_timer, &mesh_app_lpn_sds_idle_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);		
    }
    
    wiced_start_timer(&app_lpn_state.sds_idle_timer, SDS_IDLE_DELAY);
}

uint8_t sleep_timer_already_start = 0;
void mesh_app_lpn_sleep(uint32_t duration)
{
    WICED_LOG_DEBUG("%s: duration = %d\n", __func__, duration);
    memset(&app_lpn_state, 0, sizeof(app_lpn_state));

    app_lpn_state.sds_sleep_length = duration;
    if (duration != WICED_SLEEP_MAX_TIME_TO_SLEEP)
    {
        app_lpn_state.sds_sleep_length *= 1000;  // Wake-up time in us
    }

    lpn_idle_count = 0;

    // Configure to sleep as the device is idle now
    app_lpn_state.sds_sleep_config.sleep_mode           = WICED_SLEEP_MODE;
    app_lpn_state.sds_sleep_config.device_wake_mode     = WICED_SLEEP_WAKE_ACTIVE_HIGH;
    app_lpn_state.sds_sleep_config.device_wake_source   = WICED_SLEEP_WAKE_SOURCE_GPIO;
    
    //app_lpn_state.sds_sleep_config.device_wake_gpio_num = WICED_P14+WICED_P26+WICED_P27;
    app_lpn_state.sds_sleep_config.device_wake_gpio_num = WICED_P04;

    app_lpn_state.sds_sleep_config.host_wake_mode       = WICED_SLEEP_WAKE_ACTIVE_HIGH;
    app_lpn_state.sds_sleep_config.sleep_permit_handler = mesh_app_lpn_sleep_poll;

    wiced_sleep_configure(&app_lpn_state.sds_sleep_config);
    if (sleep_timer_already_start == 0)
    {   	
	    WICED_BT_TRACE("app_start_sleep_timer\n");
        sleep_timer_already_start = 1;
        if (app_lpn_state.sds_sleep_length != WICED_SLEEP_MAX_TIME_TO_SLEEP)
        {
            
            // Create wake source OSAPI timer
            osapi_createTimer(&app_lpn_state.sds_sleep_timer, mesh_app_lpn_sds_wake_timer_callback, 0);

            // Start timer
            osapi_activateTimer(&app_lpn_state.sds_sleep_timer, app_lpn_state.sds_sleep_length);
    
            // Set timer as wake source
            osapi_setTimerWakeupSource(&app_lpn_state.sds_sleep_timer, WICED_TRUE);
        }
    }
	else
	{
	    WICED_BT_TRACE("Err: sleep_timer_already_start\n");
	}
    app_lpn_state.lpn_time = clock_SystemTimeMicroseconds64();
}
// ================================================

// void mesh_app_lpn_sleep(uint32_t duration)
// {
//     WICED_LOG_DEBUG("%s: duration = %d\n", __func__, duration);

//     memset(&app_lpn_state, 0, sizeof(app_lpn_state));
    
//     app_lpn_state.lpn_state = MESH_LOW_POWER_NODE_STATE_NOT_IDLE;

//     app_lpn_state.sds_sleep_length = duration;
//     if (duration != WICED_SLEEP_MAX_TIME_TO_SLEEP)
//     {
//         app_lpn_state.sds_sleep_length *= 1000;  // Wake-up time in us
//     }

//     lpn_idle_count = 0;

//     // app_lpn_state.sds_sleep_length = SDS_SLEEP_LENGTH_MIN;
//     // app_lpn_state.sds_sleep_length = WICED_SLEEP_MAX_TIME_TO_SLEEP;

//     // Start the SDS idle wait timer
//     if(wiced_is_timer_in_use(&app_lpn_state.sds_idle_timer))
//     {
//         wiced_stop_timer(&app_lpn_state.sds_idle_timer);
//     }
//     else
//     {
//         wiced_init_timer(&app_lpn_state.sds_idle_timer, &mesh_app_lpn_sds_idle_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
//     }
    
//     wiced_start_timer(&app_lpn_state.sds_idle_timer, SDS_IDLE_DELAY);

//     // Configure to sleep as the device is idle now
//     app_lpn_state.sds_sleep_config.sleep_mode           = WICED_SLEEP_MODE;
//     app_lpn_state.sds_sleep_config.device_wake_mode     = WICED_SLEEP_WAKE_ACTIVE_HIGH;
//     app_lpn_state.sds_sleep_config.device_wake_source   = WICED_SLEEP_WAKE_SOURCE_GPIO;
    
//     //app_lpn_state.sds_sleep_config.device_wake_gpio_num = WICED_P14+WICED_P26+WICED_P27;
//     app_lpn_state.sds_sleep_config.device_wake_gpio_num = WICED_P04;

//     app_lpn_state.sds_sleep_config.host_wake_mode       = WICED_SLEEP_WAKE_ACTIVE_HIGH;
//     app_lpn_state.sds_sleep_config.sleep_permit_handler = mesh_app_lpn_sleep_poll;

//     wiced_sleep_configure(&app_lpn_state.sds_sleep_config);

//     if (app_lpn_state.sds_sleep_length != WICED_SLEEP_MAX_TIME_TO_SLEEP)
//     {
//         // Create wake source OSAPI timer
//         osapi_createTimer(&app_lpn_state.sds_sleep_timer, mesh_app_lpn_sds_wake_timer_callback, 0);

//         // Start timer
//         osapi_activateTimer(&app_lpn_state.sds_sleep_timer, app_lpn_state.sds_sleep_length);
    
//         // Set timer as wake source
//         osapi_setTimerWakeupSource(&app_lpn_state.sds_sleep_timer, WICED_TRUE);
//     }


//     app_lpn_state.lpn_time = clock_SystemTimeMicroseconds64();
// }

void mesh_lpn_SetCallback(void (*cback)(void))
{
    Sleep_Config_Cb = cback;
}

/*
 * SDS idle wait timer callback
 */
static void mesh_app_lpn_sds_idle_timer_callback(uint32_t arg)
{
    //WICED_LOG_DEBUG("%s: arg = %d\n", __func__, arg);
    //WICED_LOG_DEBUG("%s: lpn_idle_count = %d\n", __func__, lpn_idle_count);
    //app_lpn_state.lpn_state = MESH_LOW_POWER_NODE_STATE_IDLE;
    //如果按键未被释放或BLE处于连接中
    if((WICED_TRUE != andon_app_state.runing) && (!mesh_app_gatt_is_connected()))  
    {
        lpn_idle_count++;
    }
    else
    {
        lpn_idle_count = 0;
        return;
    }
    
    if(1 == lpn_idle_count)
    {
        //TODO 回调调用通知应用，即将进入休眠
        //example_key_init();
        if(Sleep_Config_Cb != NULL)
        {
            Sleep_Config_Cb();
        }
    }
    else if(3 == lpn_idle_count)
    {
        if(Sleep_Config_Cb != NULL)
        {
            Sleep_Config_Cb();
        }
        
        keyscan_stuckKey_timer_expired_callback(0);
        wiced_stop_timer(&app_lpn_state.sds_idle_timer);
        // Johnson 2020.08.01
        // ================================================
        mesh_app_lpn_sleep(record_sleep_time);
        // ================================================

        app_lpn_state.lpn_state = MESH_LOW_POWER_NODE_STATE_IDLE;
        WICED_LOG_DEBUG("%s: arg = %d\n", __func__, arg);
        WICED_LOG_DEBUG("app_lpn_state.sds_sleep_config.sleep_permit_handler = 0x%08x\n",app_lpn_state.sds_sleep_config.sleep_permit_handler);
        WICED_LOG_DEBUG("mesh_app_lpn_sleep_poll = 0x%08x\n",mesh_app_lpn_sleep_poll);
        //// #ifdef HCI_CONTROL
        //// #endif
        // wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
        // wiced_bt_mesh_core_shutdown();
    }
    else if(lpn_idle_count > 500/SDS_IDLE_DELAY)
    {
        WICED_LOG_DEBUG("wiced_hal_wdog_reset_system\n");
        wiced_hal_wdog_reset_system();
    }
}

/*
 * OSAPI SDS sleep timer callback.
 */
static void mesh_app_lpn_sds_wake_timer_callback(INT32 arg, UINT32 overTimeInUs)
{
    WICED_BT_TRACE("%s: arg = %d, overTimeInUs = %d\n", __func__, arg, overTimeInUs);

    // If the system went to SDS sleep successfully, we shall not get here!
    // If we get here, the system failed to sleep, hence let's reset here!
    wiced_hal_wdog_reset_system();
}
/*
 * Sleep permission polling time to be used by firmware
 */
//UINT64 ret = 0; 

static uint32_t mesh_app_lpn_sleep_poll(wiced_sleep_poll_type_t type)
{
    uint32_t ret = WICED_SLEEP_NOT_ALLOWED;
    

    /*
    uint32_t dev_wake_pin_state;

    dev_wake_pin_state = wiced_hal_gpio_get_pin_input_status(app_lpn_state.lpn_sleep_config.device_wake_gpio_num);
    // If device wake pin not connected to the right state (ex: for active high
    // wake should be connected to groung to allow sleep)
    if (!(dev_wake_pin_state ^ app_lpn_state.lpn_sleep_config.device_wake_mode))
    {
        WICED_LOG_DEBUG("GPIO pin does not allow sleep\n");
        return WICED_SLEEP_NOT_ALLOWED;
    }
#if 0
    else
    {
        WICED_LOG_DEBUG("%d ", dev_wake_pin_state);
    }
#endif
    */

    switch (type) {
    case WICED_SLEEP_POLL_TIME_TO_SLEEP:
        if (app_lpn_state.lpn_state != MESH_LOW_POWER_NODE_STATE_IDLE)
        {
            //WICED_LOG_DEBUG("!\n");
            ret = WICED_SLEEP_NOT_ALLOWED;
        }
        else
        {
            if (app_lpn_state.lpn_elapsed_time == 0)
            {
                app_lpn_sleep_time = clock_SystemTimeMicroseconds64();
                app_lpn_state.lpn_elapsed_time = (uint32_t)(app_lpn_sleep_time - app_lpn_state.lpn_time);
                WICED_BT_TRACE("\n%s: elapsed_time = %d us  app_lpn_sleep_time= %d us\n@\n", __func__, app_lpn_state.lpn_elapsed_time,app_lpn_sleep_time);
            }
            else
            {
                WICED_BT_TRACE("@\n");
            }
            
            //WICED_BT_TRACE("\n%s: app_lpn_sleep_time= %d us\n@\n", __func__,app_lpn_sleep_time);
            // ret = app_lpn_state.sds_sleep_length;
            LEDY_OFF;
            LEDB_OFF;
            //BTN_Indication_OFF;
            //wiced_stop_timer(&app_lpn_state.sds_idle_timer);
            ret = SDS_SLEEP_LENGTH;
            //WICED_LOG_DEBUG("%s: SDS_SLEEP_LENGTH = %d\n", __func__, ret);
        }
        break;
    case WICED_SLEEP_POLL_SLEEP_PERMISSION:
        if(andon_app_state.runing != WICED_TRUE)
        {
            // if(andon_app_state.is_provison)
            // {
            //     LEDB_ON;
            // }
            // else 
            // {
            //     LEDY_OFF;
            //     LEDB_OFF;
            // }
        }
        if (app_lpn_state.lpn_state == MESH_LOW_POWER_NODE_STATE_IDLE)
        {
#if (WICED_SLEEP_TYPE == WICED_SLEEP_TYPE_SDS)
            WICED_BT_TRACE("#\n");
            //wiced_stop_timer(&app_lpn_state.sds_idle_timer);
            ret = WICED_SLEEP_ALLOWED_WITH_SHUTDOWN;
            if (app_lpn_state.sds_sleep_length == WICED_SLEEP_MAX_TIME_TO_SLEEP)
            {
                // For maximum sleep, disable the timed wake-up
                timedWake_SDS = 0;
            }
#else
            WICED_BT_TRACE("$\n");
            ret = WICED_SLEEP_ALLOWED_WITHOUT_SHUTDOWN;
#endif
        }
        else
        {
            //WICED_LOG_DEBUG("*\n");
        }
        break;
    }
    return ret;
}
