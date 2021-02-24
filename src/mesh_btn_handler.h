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
#ifndef _MESH_BTN_HANDLER_H_
#define _MESH_BTN_HANDLER_H_

///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "wiced_hal_gpio.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif

#include "wiced_bt_btn_handler.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define ENCODER                           (1)



//按键顺序
#define BTN_ONOFF_INDEX                       (0)              
#if !ENCODER
    #define BTN_NUM                           (4)
    #define BTN_LIGHTNESS_DOWN_INDEX          (1)
    #define BTN_LIGHTNESS_UP_INDEX            (2)
    #define BTN_RESET_INDEX                   (3)   
    #define BTN_LIGHTNESS_DOWN_VALUE          (1<<BTN_LIGHTNESS_DOWN_INDEX)
    #define BTN_LIGHTNESS_UP_VALUE            (1<<BTN_LIGHTNESS_UP_INDEX)
#else
    #define BTN_NUM                           (2)
    #define BTN_RESET_INDEX                   (1)    
    #define BTN_ENCODER_MAX_CNT               200     
#endif

#define BTN_PRESSED_MAX_CNT                   5000
#define BTN_RELEASE_MAX_CNT                   5000

#define BTN_ONOFF_VALUE                       (1<<BTN_ONOFF_INDEX)
#define BTN_RESET_UP_VALUE                    (1<<BTN_RESET_INDEX)
#define BTN_MODE_PAIRED_VALUE                 0x00
#define BTN_MODE_MASK_VALUE                   0x07     

#if !ENCODER
    #define BTN_RELEASE_MASK              ( BTN_ONOFF_VALUE             \
                                            + BTN_LIGHTNESS_DOWN_VALUE  \
                                            + BTN_LIGHTNESS_UP_VALUE
                                            + BTN_RESET_UP_VALUE )
#else
    #define BTN_RELEASE_MASK              ( BTN_ONOFF_VALUE \
                                            + BTN_RESET_UP_VALUE)
#endif

#define BTN_ENCODER_MAX_CNT                  200              //
#define BTN_ENCODER_FACTOR                   (20)
#define BTN_ENCODER_MIN_STEP                 5                //

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************
typedef struct
{
    uint32_t                 btn_state_level;              //当前按键状态，随着按键分析过程而变化，按键端口状态--按键识别状态
    uint32_t                 btn_state_level0;             //上次按键状态，仅表示端口状态
    uint32_t                 btn_state_level1;             //上上次按键状态，仅表示端口状态
    uint32_t                 btn_pairpin_level;            //配对端口的状态
    uint32_t                 btn_up;                       //按键抬起
    uint32_t                 btn_down;                     //按键按下
    uint16_t                 btn_idle_count;               //所有按键抬起计数
    uint16_t                 btn_press_count[BTN_NUM];     //单个按键按住计数
    wiced_bool_t             btn_pressed;                  //是否有按键被按过
    wiced_bool_t             btn_ghost;                    //是否有keyscan异常
#if ENCODER 
    int16_t                  btn_encoder_count;            //读取的编码个数  顺时针为正，逆时针为负
#endif
} mesh_btn_handler_t;

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void mesh_btn_init(void);
void btnKeyScanStart(void);
void wiced_btn_scan(void);
///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************
extern mesh_btn_handler_t btn_state;   // Application state



#endif
