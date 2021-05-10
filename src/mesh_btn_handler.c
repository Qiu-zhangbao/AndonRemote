/*
 * Copyright 2018, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor 
 * Corporation. All rights reserved. This software, including source code, documentation and
 * related  materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its 
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
 * Implementation for generic mesh button event handling
 */
#include "spar_utils.h"
#include "wiced_bt_trace.h"
#include "wiced_hal_gpio.h"
#include "clock_timer.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif
#include "wiced_bt_btn_handler.h"
#include "wiced_hal_wdog.h"
#include "mesh_btn_handler.h"
#include "wiced_timer.h"
#include "handle_key.h"
#include "includes.h"
#include "Andon_App.h"

/******************************************************
 *          Constants
 ******************************************************/
#if CYW20735B1
extern wiced_platform_led_config_t platform_led[];
extern wiced_platform_button_config_t platform_button[];
#endif

#ifdef CYW20706A2
#define BTN_PIN    (WICED_GPIO_BUTTON)
#define LED_ON     wiced_bt_app_hal_led_on(WICED_PLATFORM_LED_1)
#define LED_OFF    wiced_bt_app_hal_led_off(WICED_PLATFORM_LED_1)
#endif

#if CYW20735B1
#define BTN_PIN    (WICED_PLATFORM_BUTTON_1)
//#define LED_OFF     wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_2].gpio, GPIO_PIN_OUTPUT_LOW)
//#define LED_ON    wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_2].gpio, GPIO_PIN_OUTPUT_HIGH)
//#define BTN_Indication_ON     wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_1].gpio, GPIO_PIN_OUTPUT_HIGH)
//#define BTN_Indication_OFF    wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_1].gpio, GPIO_PIN_OUTPUT_LOW)
#endif

#ifdef CYW20719B1
#define BTN_PIN    (WICED_PLATFORM_BUTTON_1)
#define LED_ON     wiced_bt_app_hal_led_on()
#define LED_OFF    wiced_bt_app_hal_led_off()
#endif

#define TAG  "btn handle"
#define WICED_LOG_LEVEL   WICDE_DEBUG_LEVEL

#define BTN_1                             WICED_PLATFORM_BUTTON_1
#define BTN_2                             WICED_PLATFORM_BUTTON_2
#define BTN_3                             WICED_PLATFORM_BUTTON_3
#define BTN_4                             WICED_PLATFORM_BUTTON_4
#define BTN_5                             WICED_PLATFORM_BUTTON_5

#define ENCODE_HALF_WAVE  (0)
//#define ENCODE_EDGE_TWO

/******************************************************
 *          Function Prototypes
 ******************************************************/
static void mesh_app_handle_interrupt(void* user_data, uint8_t pin, uint32_t value);
static void handler_keyscan(uint16_t keycode,uint16_t status);

extern void andon_app_encoder_action(void);
/******************************************************
 *          Structures
 ******************************************************/
typedef struct
{
    uint16_t                 btn_onoff_level;              //硬件扫描的ONOFF按键状态
    uint16_t                 btn_encode1_level;            //硬件扫描的编码器端口1的状态
    uint16_t                 btn_encode2_level;            //硬件扫描的编码器端口2的状态
} handle_hscankey_t;

/******************************************************
 *          Variables Definitions
 ******************************************************/
mesh_btn_handler_t btn_state = {0};   // Application state


static uint16_t fristToggle = 0;
static uint32_t toggleTimeKeep = 0;
static int16_t delta_plus_stata = 0;
static handle_hscankey_t handle_hscankeyvalue = {0};
static uint16_t onoff_stata = 0;                           // ON/OFF 按键的状态
static wiced_bool_t scankeystata = WICED_FALSE;            // scankey 是否正在运行
/******************************************************
 *               Function Definitions
 ******************************************************/
extern UINT32 keyscan_stuck_key_in_second;
extern void (*keyscan_stuckKey_callbackApp)(void);

void lowpower_stuckKey_callbackApp(void) 
{
    #define iocfg_p0_adr                                   0x00338200                                                   // lhl_adr_base + 0x00000200
    #define iocfg_fcn_p0_adr                               0x00338400         
 
    UINT32 pin;
 
    for (pin = 1; pin < 3; pin++)
    {
        REG32(iocfg_p0_adr + 4*(pin)) = GPIO_PULL_UP_DOWN_NONE | GPIO_EDGE_TRIGGER | GPIO_INTERRUPT_ENABLE; //0x0409;           
        REG32(iocfg_fcn_p0_adr + 4*(pin)) = 0x0000;  //Keyscan not selected
    }

    WICED_LOG_DEBUG("stuck key........\n");
    handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_IDLE;
    delta_plus_stata = 0;
    onoff_stata = 0;
    scankeystata = WICED_FALSE;
}
    
//*****************************************************************************
// 函数名称: mesh_btn_init
// 函数描述: 按键初始化
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void mesh_btn_init(void)
{
    uint32_t value;
    
    //keyscan_stuck_key_in_second = 2;
    keyscan_stuckKey_callbackApp = lowpower_stuckKey_callbackApp;

    mesh_app_interrupt_cb = mesh_app_handle_interrupt;
    WICED_LOG_DEBUG("Initial btn_state\r\n");

    memset(&btn_state,0,sizeof(mesh_btn_handler_t));
    //value = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_1].gpio);
    value =  BTN_ONOFF_VALUE;
    #if !ENCODER
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_2].gpio)<<1;
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_3].gpio)<<2;
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_4].gpio)<<3;
    #else
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_4].gpio)<<1;
    #endif

    btn_state.btn_state_level0 = BTN_RELEASE_MASK;
    btn_state.btn_state_level1 = BTN_RELEASE_MASK;
    btn_state.btn_state_level = BTN_RELEASE_MASK&value;
    
    btn_state.btn_pairpin_level = 1;
    btn_state.btn_pairpin_level += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_5].gpio);
    //唤醒时检测到按键
    if( BTN_RELEASE_MASK != (btn_state.btn_state_level&BTN_RELEASE_MASK))
    {
        WICED_LOG_DEBUG("btn is pressed during wakeup :%x\r\n",btn_state.btn_state_level);
        btn_state.btn_state_level0 = btn_state.btn_state_level;
        btn_state.btn_idle_count = 0;
        btn_state.btn_pressed = WICED_TRUE;
        for(uint16_t i=0; i<BTN_NUM; i++)
        {
            btn_state.btn_press_count[i] = 2;
        }
    }
    
    btn_state.btn_pairpin_level <<= 1;
    btn_state.btn_pairpin_level += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_5].gpio);
    btn_state.btn_pairpin_level <<= 1;
    btn_state.btn_pairpin_level += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_5].gpio);

    delta_plus_stata = 0;
    example_key_init(handler_keyscan);
    WICED_LOG_DEBUG("Pin stata: 0x%02x, mode pin:0x%02x\r\n",btn_state.btn_state_level,btn_state.btn_pairpin_level);
    WICED_LOG_DEBUG("mesh_app_interrupt_cb = 0x%X\n", mesh_app_interrupt_cb);
    scankeystata = WICED_TRUE;
    onoff_stata = 0;
}

//*****************************************************************************
// 函数名称: btnKeyScanStart
// 函数描述: 在keyScan需要保持开启时，循环调用，以防止keyscan超时停止检测
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void btnKeyScanStart(void)
{
    if(scankeystata){
        return;
    }else{
        example_key_init(handler_keyscan);
        scankeystata = WICED_TRUE;
    }
}
//*****************************************************************************
// 函数名称: wiced_btn_scan
// 函数描述: 按键扫描
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void wiced_btn_scan(void)
{
    UINT32 value;
    
    value = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_1].gpio);
    #if !ENCODER
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_2].gpio)<<1;
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_3].gpio)<<2;
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_4].gpio)<<3;
    #else
        value += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_4].gpio)<<1;
    #endif
    btn_state.btn_state_level = BTN_RELEASE_MASK&value;

    btn_state.btn_pairpin_level <<= 1;
    btn_state.btn_pairpin_level += wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_5].gpio);

    if(btn_state.btn_press_count[BTN_ONOFF_INDEX] < 20000/ANDON_APP_PERIODIC_TIME_LENGTH)
    {
        if(handle_hscankeyvalue.btn_onoff_level == HANDLE_KEY_DOWN) 
        {
            btn_state.btn_state_level &= (BTN_ONOFF_VALUE^BTN_RELEASE_MASK);
        }
        else
        {
            btn_state.btn_state_level |= BTN_ONOFF_VALUE;
            //handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_IDLE;
        }
    }
    else
    {
        btn_state.btn_state_level |= BTN_ONOFF_VALUE;
        handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_IDLE;
        onoff_stata = 0;
    }
    

    //所有按键都释放
    if(BTN_RELEASE_MASK == btn_state.btn_state_level)
    {
        if(++btn_state.btn_idle_count > BTN_RELEASE_MAX_CNT)
        {
            btn_state.btn_idle_count = BTN_RELEASE_MAX_CNT;
        }
        if(btn_state.btn_idle_count > 50)
        {
            delta_plus_stata = 0;
            btn_state.btn_pressed = WICED_FALSE;
        }
    }
    else
    {
        btn_state.btn_pressed = WICED_TRUE;
        btn_state.btn_idle_count = 0;
    }

    btn_state.btn_up = ( (btn_state.btn_state_level&btn_state.btn_state_level0&BTN_RELEASE_MASK) \
                         & (btn_state.btn_state_level1^BTN_RELEASE_MASK)) & BTN_RELEASE_MASK;
    btn_state.btn_down = ( ((btn_state.btn_state_level|btn_state.btn_state_level0)^BTN_RELEASE_MASK)
                           & (btn_state.btn_state_level1&BTN_RELEASE_MASK)) & BTN_RELEASE_MASK;
    
   
    for(uint16_t i=0; i<BTN_NUM;i++)
    {
        if(!(btn_state.btn_state_level & (1<<i) ))
        {
            btn_state.btn_press_count[i] ++;
            if(btn_state.btn_press_count[i] > BTN_PRESSED_MAX_CNT)
            {
                btn_state.btn_press_count[i] = BTN_PRESSED_MAX_CNT;
            }
        }
        else if(btn_state.btn_state_level1 & btn_state.btn_state_level0 & (1<<i))
        {
            btn_state.btn_press_count[i] = 0;
        }
    }
    btn_state.btn_state_level1 = btn_state.btn_state_level0;
    btn_state.btn_state_level0 = btn_state.btn_state_level;

    if(handle_hscankeyvalue.btn_onoff_level == HANDLE_KEY_UP)
    {
        WICED_LOG_DEBUG("toggle pressed\n");
        if(onoff_stata){
            onoff_stata--;
            btn_state.btn_up |= BTN_ONOFF_VALUE;
            btn_state.btn_press_count[BTN_ONOFF_INDEX] += 5;
        }
        if(onoff_stata == 0){
            handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_IDLE;
        }
    }
    else 
    {
        btn_state.btn_up &= (BTN_ONOFF_VALUE^BTN_RELEASE_MASK);
        if(handle_hscankeyvalue.btn_onoff_level != HANDLE_KEY_DOWN)
        {
            btn_state.btn_press_count[BTN_ONOFF_INDEX] = 0;
        }
    }

    
}


void handler_keyscan(uint16_t keycode,uint16_t status)
{
    
    btn_state.btn_idle_count = 0;
    btn_state.btn_pressed = WICED_TRUE;
    WICED_LOG_DEBUG("keycode = 0x%x %s\n",keycode,(status==HANDLE_KEY_DOWN)?"down":"up");
    //WICED_LOG_DEBUG("fristToggle : %d  delta_plus_stata : %d \n",fristToggle, delta_plus_stata);
    if(keycode == HANDLE_KEY_ONOFF)
    {
        if(status == HANDLE_KEY_DOWN)
        {
            handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_DOWN;
            onoff_stata += 1;
        }
        else if(status == HANDLE_KEY_UP)
        {
            //WICED_LOG_DEBUG("handle_hscankeyvalue.btn_onoff_level = 0x%x\n",handle_hscankeyvalue.btn_onoff_level);
            if(handle_hscankeyvalue.btn_onoff_level == HANDLE_KEY_DOWN)
            {
                handle_hscankeyvalue.btn_onoff_level = HANDLE_KEY_UP;
                //WICED_LOG_DEBUG("handle_hscankeyvalue.btn_onoff_level = 0x%x\n",handle_hscankeyvalue.btn_onoff_level);
            }
        }
    }
    else if(keycode == 0xFF)
    {
        btn_state.btn_ghost = WICED_TRUE;
    }
    #if ENCODE_HALF_WAVE
    else if(keycode == HANDLE_KEY_ENCODE_1)
    {
        if(delta_plus_stata == 0)
        {
            delta_plus_stata = 1;
        }
        else if(delta_plus_stata == -1)
        {
            andon_app_encoder_action();
            btn_state.btn_encoder_count  --;
            if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
            delta_plus_stata = 0;
        }
    }
    else if(keycode == HANDLE_KEY_ENCODE_2)
    {
        if(delta_plus_stata == 0)
        {
            delta_plus_stata = -1;
        }
        else if(delta_plus_stata == 1)
        {
            andon_app_encoder_action();
            btn_state.btn_encoder_count  ++;
            if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
            
        }
    }
    #else
    else if(keycode == HANDLE_KEY_ENCODE_1)
    {
        uint32_t temp;

        temp = clock_SystemTimeMicroseconds32();
        if((toggleTimeKeep != 0) && ((temp - toggleTimeKeep) > 1000000))
        {
            delta_plus_stata = 0;
            fristToggle = 0;
        }
        toggleTimeKeep = temp;
        //WICED_LOG_DEBUG("fristToggle : %d  delta_plus_stata : %d toggleTimeKeep : %d\n",fristToggle, delta_plus_stata,toggleTimeKeep/1000);
        
        if(fristToggle == 0)
        {  
            if(status == HANDLE_KEY_DOWN)
            {
                if(delta_plus_stata != -1)
                {
                    delta_plus_stata = 1;
                }
                else 
                {
                    delta_plus_stata = -2;
                }
            }  
            else if(status == HANDLE_KEY_UP)
            {
                if(delta_plus_stata == -3)
                {
                    fristToggle = 1;
                    andon_app_encoder_action();
                    btn_state.btn_encoder_count  --;
                    if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                        btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
                    delta_plus_stata = 0;
                }
                else if(delta_plus_stata == 2)
                {
                    delta_plus_stata = 3;
                }
                else if(delta_plus_stata == 1)
                {
                    delta_plus_stata = 0;
                }
            }
        }
        else
        {
            if((delta_plus_stata == 0) && (status == HANDLE_KEY_DOWN))
            {
                 delta_plus_stata = 1;
            }
            else if((delta_plus_stata == -1) && (status == HANDLE_KEY_DOWN))
            {
                andon_app_encoder_action();
                btn_state.btn_encoder_count  --;
                if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
                delta_plus_stata = 0;
            }
            else if((status == HANDLE_KEY_UP) && (delta_plus_stata == -1))
            {
                delta_plus_stata = 0;
            }
        }
        
        // if(delta_plus_stata == 0)
        // {
        //     if(status == HANDLE_KEY_UP)
        //     {
        //         delta_plus_stata = 1;
        //     }
        // }
        // else if(delta_plus_stata == -1)
        // {
        //     if(status == HANDLE_KEY_UP)
        //     {
        //         andon_app_encoder_action();
        //         btn_state.btn_encoder_count  --;
        //         if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
        //             btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
        //     }
        //     delta_plus_stata = 0;
        // }
        
    }
    else if(keycode == HANDLE_KEY_ENCODE_2)
    {
        uint32_t temp;
        temp = clock_SystemTimeMicroseconds32();
        if((toggleTimeKeep != 0) && ((temp - toggleTimeKeep) > 1000000))
        {
            delta_plus_stata = 0;
            fristToggle = 0;
        }
        toggleTimeKeep = temp;
        //WICED_LOG_DEBUG("fristToggle : %d  delta_plus_stata : %d toggleTimeKeep : %d\n",fristToggle, delta_plus_stata,toggleTimeKeep/1000);

        if(fristToggle == 0)
        {
            if(status == HANDLE_KEY_DOWN)
            {
                if(delta_plus_stata != 1)
                {
                    delta_plus_stata = -1;
                }
                else 
                {
                    delta_plus_stata = 2;
                }
            }  
            else if(status == HANDLE_KEY_UP)
            {
                if(delta_plus_stata == 3)
                {
                    fristToggle = 1;
                    andon_app_encoder_action();
                    btn_state.btn_encoder_count  ++;
                    if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                        btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
                    delta_plus_stata = 0;
                }
                else if(delta_plus_stata == -2)
                {
                    delta_plus_stata = -3;
                }
                else if(delta_plus_stata == -1)
                {
                    delta_plus_stata = 0;
                }
            }
        }
        else
        {
            if((delta_plus_stata == 0) && (status == HANDLE_KEY_DOWN))
            {
                 delta_plus_stata = -1;
            }
            else if((delta_plus_stata == 1) && (status == HANDLE_KEY_DOWN))
            {
                andon_app_encoder_action();
                btn_state.btn_encoder_count  ++;
                if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
                delta_plus_stata = 0;
            }
            else if((status == HANDLE_KEY_UP) && (delta_plus_stata == 1))
            {
                delta_plus_stata = 0;
            }
        }
        
        // if(delta_plus_stata == 0)
        // {
        //     if(status == HANDLE_KEY_UP)
        //     {
        //         delta_plus_stata = -1;
        //     }
        // }
        // else if(delta_plus_stata == 1)
        // {
        //     if(status == HANDLE_KEY_UP)
        //     {
        //         andon_app_encoder_action();
        //         btn_state.btn_encoder_count  ++;
        //         if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
        //             btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
        //     }
        //     delta_plus_stata = 0;
        // }
    }
    #endif
}

// static uint8_t rsttimer_tick = 0;
// static wiced_timer_t btnresettimer;
// extern void AndonPair_Stop(void);

// static void mesh_btn_reset_timer_callback(uint32_t arg)
// {
//     rsttimer_tick ++;
//     if(1 == rsttimer_tick%15)
//     {
//         AndonPair_Stop();
//     }
//     if(rsttimer_tick == 40)
//     {
//         //使用连续两次初始化同一个定时器进行复位，这样启动速度比使用系统复位函数启动较快
//         wiced_init_timer(&btnresettimer, &mesh_btn_reset_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
//     }
//     else if(rsttimer_tick > 50)
//     {
//         wiced_stop_timer(&btnresettimer);
//         wiced_hal_wdog_reset_system();
//     }
    
// }

/*
 * Interrupt handled called in mesh_application.c
 */
static void mesh_app_handle_interrupt(void* user_data, uint8_t pin, uint32_t value)
{
    uint16_t  tick;
    uint16_t  tick1;

    
    // if(pin == *platform_button[BTN_5].gpio) 
    // {
    //     if(wiced_is_timer_in_use(&btnresettimer))
    //     {
    //         wiced_deinit_timer(&btnresettimer);
    //     }
    //     wiced_init_timer(&btnresettimer, &mesh_btn_reset_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
    // } 
    WICED_LOG_DEBUG("btn isr Pin : %d \n",pin);
    // if(pin == *platform_button[BTN_5].gpio)               //模式选择对应的按键
    // {
    //     // wiced_hal_wdog_reset_system();  //使用连续两次初始化同一个定时器进行复位，这样启动速度比使用系统复位函数启动较快
    //     wiced_start_timer(&btnresettimer,10);
    //     rsttimer_tick = 0;
    //     //wiced_init_timer(&btnresettimer, &mesh_btn_reset_timer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
    //     return;
    // }
#if ENCODER
#ifndef ENCODE_EDGE_TWO   //仅一个GPIO中断方式
    if(pin == *platform_button[BTN_2].gpio)         //编码器对应的端口，仅识别一个端口即可
    {
        btn_state.btn_idle_count = 0;
        tick = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_2].gpio);
        if(0 == tick)
        {
            tick = wiced_hal_gpio_get_pin_input_status((uint32_t)(*platform_button[BTN_3].gpio));
            if(0 == tick)
            {
                delta_plus_stata = 1;
            }
            else
            {
                delta_plus_stata = -1;
            }
        }
        else if(1 == tick)
        {
            tick = wiced_hal_gpio_get_pin_input_status((uint32_t)(*platform_button[BTN_3].gpio));
            if((1 == tick) && (1 == delta_plus_stata))
            {
                btn_state.btn_encoder_count ++;
                if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
                //WICED_LOG_DEBUG("btn encoder Plus count = %d", btn_state.btn_encoder_count);
            }
            else if((0 == tick) && (-1 == delta_plus_stata))
            {
                btn_state.btn_encoder_count  --;
                if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
                //WICED_LOG_DEBUG("btn encoder Minus count = %d", btn_state.btn_encoder_count);
            }
            delta_plus_stata = 0;
            btn_state.btn_idle_count = 0;
        }
    }
#else  //两个端口方式
    if(pin == *platform_button[BTN_2].gpio)         //识别其中一个端口
    {
        uint16_t tick1;
        btn_state.btn_idle_count = 0;
        tick1 = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_3].gpio);
        tick = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_2].gpio);


        if(0 == delta_plus_stata)   //正在第一个端口的脉冲
        {
            if(tick1 == tick) 
            {
                delta_plus_stata = 1;
            }
            else
            {
                delta_plus_stata = -1;
            }
        }
        else
        {
            if((1 == delta_plus_stata) && (tick1 == tick)) 
            {
                btn_state.btn_encoder_count ++;
                if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
            }
            else if((-1 == delta_plus_stata) && (tick1 != tick))
            {
                btn_state.btn_encoder_count  --;
                if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
            }
            delta_plus_stata = 0; 
        }
        WICED_LOG_DEBUG("delta_plus_stata = %d, tick = %d, tick1 = %d\n",delta_plus_stata,tick,tick1);
    }
    else if(pin == *platform_button[BTN_3].gpio)
    {
        btn_state.btn_idle_count = 0;
        tick = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_2].gpio);
        tick1 = wiced_hal_gpio_get_pin_input_status((uint32_t)*platform_button[BTN_3].gpio);

        if(0 == delta_plus_stata)   //正在第一个端口的脉冲
        {
            if(tick1 == tick) 
            {
                delta_plus_stata = 1;
            }
            else
            {
                delta_plus_stata = -1;
            }
        }
        else
        {
            if((1 == delta_plus_stata) && (tick1 == tick)) 
            {
                btn_state.btn_encoder_count ++;
                if(btn_state.btn_encoder_count  > BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = BTN_ENCODER_MAX_CNT;
            }
            else if((-1 == delta_plus_stata) && (tick1 != tick))
            {
                btn_state.btn_encoder_count  --;
                if(btn_state.btn_encoder_count  < -BTN_ENCODER_MAX_CNT)
                    btn_state.btn_encoder_count  = -BTN_ENCODER_MAX_CNT;
            }
            delta_plus_stata = 0;
        }
        
        WICED_LOG_DEBUG("delta_plus_stata = %d, tick = %d, tick1 = %d\n",delta_plus_stata,tick,tick1);
    }
    
#endif
#endif

}
