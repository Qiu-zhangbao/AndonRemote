//******************************************************************************
//*
//* 文 件 名 : AndonCmd.c
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

///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************

#include "wiced_bt_ble.h"
#include "adv_pack.h"
#include "mesh_vendor.h"
#include "AndonCmd.h"
#include "includes.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
extern wiced_bool_t mesh_app_node_is_provisioned(void);
///*****************************************************************************
///*                         常量定义区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
static int16_t delta_all = 0;

///*****************************************************************************
///*                         函数实现区
///*****************************************************************************
//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  delay 延迟时间单位ms，超过62s时，设置为0xFF
// 函数返回值: 
//*****************************************************************************/
void AndonCmd_Toggle(uint16_t delay)
{
    uint8_t pack_load[8];

    #if(ANDONCMD_TRANS_TIME > 62000)
        pack_load[0] = 62000/1000;
        pack_load[0] |= 0x40;
    #elif(ANDONCMD_TRANS_TIME > 6200)   
        pack_load[0] = ANDONCMD_TRANS_TIME/1000;
        pack_load[0] |= 0x40;
    #else
        pack_load[0] = ANDONCMD_TRANS_TIME/100;
    #endif
    
    
    if(delay > 62000)
    {
        pack_load[1] = 0xFF;
    }    
    else
    {
        if(delay > 6200)
        {
            delay = delay/1000;
            delay |= 0x40;
        }
        else
        {
            delay = delay/100;
        }
    }
    pack_load[1] = delay;

    if(mesh_app_node_is_provisioned())
    {
        mesh_vendor_send_cmd(VENDOR_SETTOGGLEONOFF_CMD,pack_load,2);
        //adv_vendor_send_cmd(ADV_CMD_SETTOGGLEONOFF_CMD,pack_load,2,ADV_PACK_TTL);
    }
    else
    {
        adv_vendor_send_cmd(ADV_CMD_SETTOGGLEONOFF_CMD,pack_load,2,ADV_PACK_TTL);
    }
    
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  onoff 1 开灯 0关灯 delay 延迟时间单位ms，超过62s时，设置为0xFF
// 函数返回值: 
//*****************************************************************************/
void AndonCmd_Onoff(wiced_bool_t onoff,uint16_t delay)
{
    uint8_t pack_load[8];
    uint8_t len;

    #if(ANDONCMD_TRANS_TIME > 62000)
        pack_load[1] = 62000/1000;  
        pack_load[1]  |= 0x40;
    #elif(ANDONCMD_TRANS_TIME > 6200)   
        pack_load[1]  = ANDONCMD_TRANS_TIME/1000;
        pack_load[1]  |= 0x40;
    #else
        pack_load[1]  = ANDONCMD_TRANS_TIME/100;
    #endif
    
    if(delay > 62000)
    {
        pack_load[2] = 0xFF;
    }    
    else
    {
        if(delay > 6200)
        {
            delay = delay/1000;
            delay |= 0x40;
        }
        else
        {
            delay = delay/100;
        }
    }
    pack_load[2] = delay;
    if(mesh_app_node_is_provisioned())
    {
        if(WICED_FALSE == onoff)
        {
            pack_load[0] = VENDOR_ONOFF_OFF;
        }
        else
        {
            pack_load[0] = VENDOR_ONOFF_ON;
        }
        
        mesh_vendor_send_cmd(VENDOR_SETONOFF_CMD,pack_load,3);
        //adv_vendor_send_cmd(ADV_CMD_SETONOFF_CMD,pack_load,2,ADV_PACK_TTL);
    }
    else
    {
        if(WICED_FALSE == onoff)
        {
            pack_load[0] = ADV_ONOFF_OFF;
        }
        else
        {
            pack_load[0] = ADV_ONOFF_ON;
        }
        adv_vendor_send_cmd(ADV_CMD_SETONOFF_CMD,pack_load,3,ADV_PACK_TTL);
    }
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonCmd_Delta(int16_t delta, uint16_t tran_time)
{
    uint8_t pack_load[8];
    uint8_t len = 0;

    if(delta > 100)
        delta = 100;
    else if(delta < -100)
        delta = -100;
    
    if(delta_all < -100 && delta > 0)
    {
        delta_all = 0;
    }

    if(delta_all > 100 && delta < 0)
    {
        delta_all = 0;
    }

    delta_all += delta;
    if(delta_all > 200)   //变化量超过变化范围的2倍，不在发送数据
    {
        delta_all = 200;
        delta = 100;
        return;
    } 
    else if(delta_all < -200) //变化量超过变化范围的2倍，不在发送数据
    {
        delta_all = -200;
        delta = -100;
        return;
    }
    
    pack_load[len++] = delta;

    if(tran_time > 62000)
        tran_time = 62000;
    if(tran_time > 6200)
    {
        tran_time = tran_time/1000;
        tran_time |= 0x40;
    }
    else
    {
        tran_time = tran_time/100;
    }
    pack_load[len++] = tran_time&0xFF;
    //pack_load[len++] = 0x00;
    
    if(mesh_app_node_is_provisioned())
    {
        mesh_vendor_send_cmd(VENDOR_DELTABRIGHTNESS_CMD,pack_load,len);
        //adv_vendor_send_cmd(ADV_CMD_DELTABRIGHTNESS,pack_load,len,ADV_PACK_TTL);
    }
    else
    {
        adv_vendor_send_cmd(ADV_CMD_DELTABRIGHTNESS,pack_load,len,ADV_PACK_TTL);
    }
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonCmd_Action(enumREMOTEACTION_t action, int16_t delta, uint16_t tran_time)
{
    uint8_t pack_load[8];
    uint8_t len = 0;

    if(action >= enumREMOTEACTION_MAX){
        return;
    }
    
    LOG_DEBUG("Send Action  %d....................\n",action);
    pack_load[len++] = (uint8_t)action;
    pack_load[len++] = delta;

    if(tran_time > 62000)
        tran_time = 62000;
    if(tran_time > 6200)
    {
        tran_time = tran_time/1000;
        tran_time |= 0x40;
    }
    else
    {
        tran_time = tran_time/100;
    }
    pack_load[len++] = tran_time&0xFF;

    if(mesh_app_node_is_provisioned()){
        mesh_vendor_send_cmd(VENDOR_REMOTEACTION_CMD,pack_load,len);
        //adv_vendor_send_cmd(ADV_CMD_DELTABRIGHTNESS,pack_load,len,ADV_PACK_TTL);
    }else{
        adv_vendor_send_cmd(ADV_CMD_REMOTEACTION,pack_load,len,ADV_PACK_TTL);
    }

}


