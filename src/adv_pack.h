//******************************************************************************
//*
//* �� �� �� : adv_pack.h
//* �ļ����� : 
//* ��    �� : zhw/Andon Health CO.LTD
//* ��    �� : V0.0
//* ��    �� : 
//* �����б� : ��.c
//* ������ʷ : ��.c           
//******************************************************************************
#ifndef __ADV_PACK_H__
#define __ADV_PACK_H__
///*****************************************************************************
///*                         �����ļ�˵��
///*****************************************************************************
#include "mesh_vendor.h"

///*****************************************************************************
///*                         �궨����
///*****************************************************************************

#define ADV_PAIRADV_INDEX        10
#define ADV_MANUDEVADV_INDEX     11

#define ADV_CMD_PARI                0x0200
// #define ADV_PARILOAD_DOPAIR         00
// #define ADV_PARILOAD_PAIRACK        01
// #define ADV_PARILOAD_PAIRED         02
// #define ADV_PARILOAD_SELECT         03


#define ADV_PARILOAD_DOPAIR       0x00
#define ADV_PARILOAD_PAIRACK      0x01
#define ADV_PARILOAD_PAIRED       0x02
#define ADV_PARILOAD_SELECT       0x03
#define ADV_PARILOAD_PAIREDACK    0x04
#define ADV_PARILOAD_WAKEUP       0x07
#define ADV_PARILOAD_RESET        0xFF

#define ADV_CMD_REMOTEACTION        VENDOR_REMOTEACTION_CMD 
#define ADV_CMD_DELTABRIGHTNESS     VENDOR_DELTABRIGHTNESS_CMD   
#define ADV_CMD_SETONOFF_CMD        VENDOR_SETONOFF_CMD
#define ADV_CMD_SETTOGGLEONOFF_CMD  VENDOR_SETTOGGLEONOFF_CMD

#define ADV_CMD_RESET               0x02FF
#define ADV_CMD_TOOLING             0xFF01
#define ADV_TOOLING_BURNIN          0x01
#define ADV_TOOLING_AFTERBURN       0x02
#define ADV_TOOLING_POWER_TEST      0x03
#define ADV_TOOLING_UPGRADE_VER     0x04

#define ADV_ONOFF_ON                VENDOR_ONOFF_ON     
#define ADV_ONOFF_OFF               VENDOR_ONOFF_OFF                     

#define ADV_PACK_TTL                1

///*****************************************************************************
///*                         Strcut�ṹ����������
///*****************************************************************************
typedef void (*wiced_ble_adv_cmd_cback_t) (void *, uint16_t , void *,uint8_t);

///*****************************************************************************
///*                         ����������
///*****************************************************************************
void adv_pack_init(void);
void adv_vendor_send_cmd(uint16_t user_cmd,uint8_t *pack_load,uint8_t len,uint8_t ttl);
void adv_manuDevAdvStart(uint8_t *data, uint8_t len);
void adv_manuDevAdvStop(void);
void advpackReadRemoteIndex(void);
void advpackReWriteRemoteIndex(void);
void advpackResetRemoteIndex(void);
///*****************************************************************************
///*                         �ⲿȫ�ֱ���������
///*****************************************************************************

#endif 
