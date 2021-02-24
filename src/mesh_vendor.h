//******************************************************************************
//*
//* 文 件 名 : mesh_vendor.h
//* 文件描述 : 
//* 作    者 : zhw/Andon Health CO.LTD
//* 版    本 : V0.0
//* 日    期 : 
//* 函数列表 : 见.c
//* 更新历史 : 见.c           
//******************************************************************************
#ifndef __MESH_VENDOR_H__
#define __MESH_VENDOR_H__
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define MESH_VENDOR_ELEMENT_INDEX       0

// This sample shows simple use of vendor get/set/status messages.  Vendor model
// can define any opcodes it wants.
#define MESH_VENDOR_OPCODE_GET          1       // Command to Get data
#define MESH_VENDOR_OPCODE_SET          2       // Command to Set data ack is required
#define MESH_VENDOR_OPCODE_SET_UNACKED  3       // Command to Set data no ack is required
#define MESH_VENDOR_OPCODE_STATUS       4       // Response from the server, or unsolicited data change.

#define VENDOR_DELTABRIGHTNESS_CMD   0x0302
#define VENDOR_SETONOFF_CMD          0x0201
#define VENDOR_SETTOGGLEONOFF_CMD    0x0401
#define VENDOR_REMOTEACTION_CMD      0x020B

#define VENDOR_ONOFF_ON                      ((uint8_t)(1))
#define VENDOR_ONOFF_OFF                     ((uint8_t)(0))


#define ACTION_GET (0x01)
#define ACTION_SET (0x02)
#define ACTION_DELTA (0x03)
#define ACTION_TOGGLE (0x04)
#define ACTION_START (0x05)
#define ACTION_STATUS (0x06)


#define _countof(array) (sizeof(array) / sizeof(array[0]))

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void mesh_vendor_send_cmd(uint16_t user_cmd,uint8_t *pack_load,uint8_t length);

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

#endif 
