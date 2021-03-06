﻿//******************************************************************************
//*
//* 文 件 名 : includes.h
//* 文件描述 :        
//* 作    者 : 张威/Andon Health CO.Ltd
//* 版    本 : V0.0
//* 日    期 : 
//******************************************************************************
#ifndef _INCLUDES_H_
#define _INCLUDES_H_
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "wiced_bt_ble.h"
#include "wiced_bt_btn_handler.h"
#include "log.h"
#include "display.h"
#include "app_config.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************



///*****************************************************************************
///*                         数据类型重定义去
///*****************************************************************************

typedef wiced_bool_t (wiced_bt_ble_userscan_result_cback_t) (wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data);

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局常量声明区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量声明区
///*****************************************************************************
extern mesh_app_interrupt_handler_t mesh_app_interrupt_cb;

#endif