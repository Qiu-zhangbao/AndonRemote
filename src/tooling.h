//******************************************************************************
//*
//* 文 件 名 : tooling.h
//* 文件描述 : 
//* 作    者 : zhw/Andon Health CO.LTD
//* 版    本 : V0.0
//* 日    期 : 
//* 函数列表 : 见.c
//* 更新历史 : 见.c           
//******************************************************************************
#pragma once

#ifndef __TOOLING_H__
#define __TOOLING_H__
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "stdint.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define TOOLING_BURNIN_IDLE                      0
#define TOOLING_BURNIN_STEP1                     1
#define TOOLING_BURNIN_STEP1_ERR                 2
#define TOOLING_BURNIN_STEP2                     3
#define TOOLING_BURNIN_STEP3                     4
#define TOOLING_BURNIN_STEP3_CONTINUE            5
#define TOOLING_BURNIN_DONE                      0xFF

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************
typedef union {
    struct tool_stru
    {
        uint16_t burningFlag;
        uint16_t burninMin;
        uint16_t burninMax;
    } item;
    uint8_t burnin_array[sizeof(struct tool_stru)];
} tooling_burnin_t;
///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void tooling_init(void);
void toolSmtCheck(void);
//void tooling_handle(wiced_bt_ble_scan_results_t *p_scan_result,uint16_t cmd, uint8_t *data, uint8_t len);

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

#endif 
