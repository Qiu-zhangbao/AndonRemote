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

#ifdef ANDON_BEEP

#define PORT_PWM                     WICED_P32
#define PWM_CHANNEL                  PWM1    
#define WICED_PWM_CHANNEL            WICED_PWM1
#define PWM_INP_CLK_IN_HZ            (1024 * 3000)
#define PWM_FREQ_IN_HZ               (3000)
#define BEEP_PERIODIC_TIME_LENGTH    50
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "wiced_timer.h"
#include "wiced_platform.h"
#include "wiced_hal_pwm.h"
#include "wiced_hal_aclk.h"
#include "Beep.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************
typedef struct{
    uint32_t duration;
    uint32_t interval;
    uint32_t cycle;
}beep_t;

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************

///*****************************************************************************
///*                         常量定义区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
beep_t beepset;
wiced_timer_t beep_timer;

///*****************************************************************************
///*                         函数实现区
///*****************************************************************************
void beep_on(void)
{
    pwm_config_t pwm_config;
    
    wiced_hal_gpio_select_function(PORT_PWM,WICED_GPIO);
    wiced_hal_gpio_configure_pin(PORT_PWM, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
    wiced_hal_aclk_enable(PWM_INP_CLK_IN_HZ, ACLK1, ACLK_FREQ_24_MHZ);
    wiced_hal_gpio_select_function(PORT_PWM,WICED_PWM_CHANNEL);
    wiced_hal_pwm_get_params(PWM_INP_CLK_IN_HZ, 50, PWM_FREQ_IN_HZ, &pwm_config);
    wiced_hal_pwm_start(PWM_CHANNEL, PMU_CLK, pwm_config.toggle_count, pwm_config.init_count, 0);
    wiced_hal_pwm_enable(PWM_CHANNEL);
}

void beep_off(void)
{
    wiced_hal_pwm_disable(PWM_CHANNEL);
    wiced_hal_gpio_select_function(PORT_PWM,WICED_GPIO);
    wiced_hal_gpio_configure_pin(PORT_PWM, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
}

void Beep_Periodic_Timer_Callback(uint32_t arg)
{
    beepset.cycle--;
    if(beepset.cycle == 0)
    {
        AnondBeepOff();
        return;
    }
    if((beepset.cycle%beepset.interval) == beepset.duration)
    {
        beep_off();
    }
    else if ((beepset.cycle%beepset.interval) == 0)
    {
        beep_on();
    }
    
}
//*****************************************************************************
// 函数名称: AnondBeepOff
// 函数描述: 强制关闭蜂鸣器
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AnondBeepOff(void)
{
    beepset.duration = 0;
    beepset.interval = 0;
    beepset.cycle = 0;

    beep_off();
    if(wiced_is_timer_in_use(&beep_timer))  
    {
        wiced_stop_timer(&beep_timer);
    }
}

//*****************************************************************************
// 函数名称: AnondBeepOn
// 函数描述: 开始蜂鸣
// 函数输入:  times：蜂鸣次数  duration：蜂鸣时间单位ms  uint16_t 蜂鸣间隔单位ms
// 函数返回值: 
//*****************************************************************************/
void AnondBeepOn(uint8_t times, uint16_t duration, uint16_t interval)
{

    //采用计数减操作，先响后停止，但计数从大减小，
    beepset.interval = (interval+duration)/BEEP_PERIODIC_TIME_LENGTH;
    beepset.duration = interval/BEEP_PERIODIC_TIME_LENGTH;
    beepset.cycle    = beepset.interval*times;

    if(wiced_is_timer_in_use(&beep_timer))  
    {
        wiced_stop_timer(&beep_timer);
    }
    wiced_init_timer(&beep_timer, &Beep_Periodic_Timer_Callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
    wiced_start_timer(&beep_timer, BEEP_PERIODIC_TIME_LENGTH);
    beep_on();
}

#endif

