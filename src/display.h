//******************************************************************************
//*
//* 文 件 名 : 
//* 文件描述 :        
//* 作    者 : 张威/Andon Health CO.Ltd
//* 版    本 : V0.0
//* 日    期 : 
//******************************************************************************
#ifndef __DISPLAY_H__
#define __DISPLAY_H__
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define ANDON_APP_DISPLAY_NORMAL                    0x0000           //普通模式
//#define ANDON_APP_DISPLAY_BTN_PRESS                 0x0001           //按键按下或旋转编码器
#define ANDON_APP_DISPLAY_BTN_RESET3S               0x0002           //按键RESET键3s
#define ANDON_APP_DISPLAY_BTN_RESET10S              0x0003           //按键RESET键10s
#define ANDON_APP_DISPLAY_FLASH                     0x0004           //入网前后的闪灯
#define ANDON_APP_DISPLAY_CONFIG                    0x0005           //网络配置模式
#define ANDON_APP_DISPLAY_BTN_PRESS_NORMAL          0X0006           //普通模式下按键按下或旋转编码器
#define ANDON_APP_DISPLAY_BTN_PRESS_CONFIG          0X0007           //配置模式下按键按下或旋转编码器
#define ANDON_APP_DISPLAY_LOWPOWER                  0x00FF           //低电压
#define ANDON_APP_DISPLAY_PAIR_DOING                0x0100           //配对中
#define ANDON_APP_DISPLAY_PAIR_SUCCESS              0x0101           //配对成功
#define ANDON_APP_DISPLAY_PAIR_FAILED               0x0102           //配对失败
#define ANDON_APP_DISPLAY_PROVISION_DOING           0x0200           //provisioning
#define ANDON_APP_DISPLAY_PROVISION_DONE            0x0201           //provisioned
#define ANDON_APP_DISPLAY_TEST_DOING                0xFF00           //测试过程中
#define ANDON_APP_DISPLAY_TEST_BLEOK                0xFF01           //ble测试通过
#define ANDON_APP_DISPLAY_TEST_BLEFAILED            0xFF02           //ble测试通过
#define ANDON_APP_DISPLAY_TEST_MODEFAILED           0xFF03           //型号校验未通过
#define ANDON_APP_DISPLAY_TEST_DIDFAILED            0xFF04           //DID校验未通过
#define ANDON_APP_DISPLAY_TEST_UPGRADEOK            0xFF05           //配合工装升级完成            




///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void DisplayRefresh(wiced_bool_t is_provison);
void DisplayInit(void);
void DisplayDeinit(void);
///*****************************************************************************
///*                         外部全局常量声明区
///*****************************************************************************


#endif