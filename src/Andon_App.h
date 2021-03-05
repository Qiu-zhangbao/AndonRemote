//******************************************************************************
//*
//* 文 件 名 : 
//* 文件描述 : 
//* 作    者 : zhw/Andon Health CO.LTD
//* 版    本 : V0.0
//* 日    期 : 
//* 函数列表 : 见.c
//* 更新历史 : 见.c           
//******************************************************************************
#ifndef _ANDON_APP_H_
#define _ANDON_APP_H_

///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "wiced_hal_gpio.h"
#include "wiced_timer.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************

#if CYW20735B1
extern wiced_platform_led_config_t platform_led[];
#endif

#ifdef CYW20706A2
#define BTN_PIN    (WICED_GPIO_BUTTON)
#define LED_ON     wiced_bt_app_hal_led_on(WICED_PLATFORM_LED_1)
#define LED_OFF    wiced_bt_app_hal_led_off(WICED_PLATFORM_LED_1)
#endif

#if CYW20735B1
#define BTN_PIN    (WICED_PLATFORM_BUTTON_1)
#define LEDB_ON     wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_2].gpio, GPIO_PIN_OUTPUT_LOW)
#define LEDB_OFF    wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_2].gpio, GPIO_PIN_OUTPUT_HIGH)
#define LEDY_ON     wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_1].gpio, GPIO_PIN_OUTPUT_LOW)
#define LEDY_OFF    wiced_hal_gpio_set_pin_output((uint32_t)*platform_led[WICED_PLATFORM_LED_1].gpio, GPIO_PIN_OUTPUT_HIGH)
#endif

#ifdef CYW20719B1
#define BTN_PIN    (WICED_PLATFORM_BUTTON_1)
#define LED_ON     wiced_bt_app_hal_led_on()
#define LED_OFF    wiced_bt_app_hal_led_off()
#endif

#define ANDON_APP_PERIODIC_TIME_LENGTH              20               //ms

#define ANDON_APP_RUN_NORMAL                        0                //正常工作模式
#define ANDON_APP_RUN_WAIT_PROVISION                1                //等待配网
#define ANDON_APP_RUN_APP_PROVISION                 2                //App入网
#define ANDON_APP_RUN_BAKGROUD_PROVISION            3                //后台配网
#define ANDON_APP_RUN_TEST                          0xFF             //测试模式



///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************
typedef struct
{
    wiced_timer_t            periodic_timer;      //按键扫描定时器
    wiced_bool_t             runing;              //是否处于运行中，为TRUE时不允许进行低功耗状态
    wiced_bool_t             is_provison;         //是否入网
    uint32_t                 run_mode;            //当前运行状态
    uint32_t                 display_mode;        //显示状态
}andon_app_handler_t;

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void Andon_App_Init(wiced_bool_t);
wiced_bool_t appAndonBleConnectUsed(void);
void appBleConnectNotify(wiced_bool_t isconneted,wiced_bt_device_address_t);
void appSetConnAdvEnable(void);
void appSetConnAdvDisable(void);
void AppGetMyMac(wiced_bt_device_address_t p_data);
///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************
extern andon_app_handler_t andon_app_state;

#endif
