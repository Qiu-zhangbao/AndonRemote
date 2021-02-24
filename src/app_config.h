//******************************************************************************
//*
//* 文 件 名 : app_config.h
//* 文件描述 :        
//* 作    者 : 张威/Andon Health CO.Ltd
//* 版    本 : V0.0
//* 日    期 : 
//******************************************************************************
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "wiced_bt_mesh_models.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
//#define USE_REMOTE_PROVISION    (1)  // Enable remote provisioning server
#define MESH_DFU                (0)              // Enable Mesh DFU


#define MESH_CID                0x0804
#define MESH_PID                (0x4130)  //"A0"的ACSII

#define ANDON_HW_MAJOR          0         //0--9
#define ANDON_HW_MINOR          0         //0--9
#define ANDON_HW_PATCH          1        //0--127
#define MESH_VID                ((ANDON_HW_MAJOR<<12)&0xF000 | (ANDON_HW_MINOR<<8)&0x0F00 | ANDON_HW_PATCH&0xFF)
// #define MESH_VID      (0x0101)

#define ANDON_FW_MAJOR          0         //0--9
#define ANDON_FW_MINOR          0         //0--9
#define ANDON_FW_PATCH          0x13      //0--127
#define VERSION                 ((ANDON_FW_MAJOR<<12)&0xF000 | (ANDON_FW_MINOR<<8)&0x0F00 | ANDON_FW_PATCH&0xFF)
#if ANDON_TEST
    #define MESH_FWID (VERSION|0x80)
#else
    #define MESH_FWID (VERSION&0XFF7F)
#endif
#define MESH_CACHE_REPLAY_SIZE  0x0080

#define MANUFACTURER_CODE       "Andon"
#define PRODUCT_CODE            "JA_SLA0"

#define MESH_VENDOR_COMPANY_ID          0x804   // Cypress Company ID
#define MESH_VENDOR_MODEL_ID            1       // ToDo.  This need to be modified
///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局常量声明区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量声明区
///*****************************************************************************
extern wiced_bt_mesh_core_config_t  mesh_config;

#endif