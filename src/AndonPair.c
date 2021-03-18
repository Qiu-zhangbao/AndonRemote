//******************************************************************************
//*
//* 文 件 名 : AndonPair.c
//* 文件描述 :  遥控器与灯间配对过程的相关处理      
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
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "adv_pack.h"
#include "AndonPair.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_mesh_event.h"
#include "wiced_bt_mesh_core.h"
#include "includes.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define TAG  "AndonPair"
#define WICED_LOG_LEVEL   WICDE_DEBUG_LEVEL

#define ADV_PAIR_DEVICE_MAXNUM        20
///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************
typedef struct{
    wiced_bool_t             AndonPair_doing;
    wiced_timer_t            AndonPair_timer;
    uint32_t                 AndonPair_timer_cnt;
    AndonPair_Done_cback_t  *AndonPair_Done_cback;
    wiced_bool_t             AndonPair_SelectLightFlag;
}AndonPair_handle_t;

typedef struct
{
    uint8_t                   pair_stata;
    wiced_bt_device_address_t dst_mac;
} mesh_btn_pairload_t;

typedef struct
{
    wiced_bt_device_address_t  pair_device_cache[ADV_PAIR_DEVICE_MAXNUM];
    uint16_t                   current_pair_cacheIndex;
    uint16_t                   max_pair_cacheIndex;
} mesh_paircache_t;

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
extern void AppGetMyMac(wiced_bt_device_address_t p_data);

///*****************************************************************************
///*                         常量定义区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
static AndonPair_handle_t AndonPair_handle;
static mesh_paircache_t   AndonPair_Cache;
static wiced_bt_device_address_t paried_addr;

///*****************************************************************************
///*                         函数实现区
///*****************************************************************************

//*****************************************************************************
// 函数名称: AndonPair_SendPairStart
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static inline void AndonPair_SendPairStart(void)
{
    uint8_t pack_load;
    pack_load = ADV_PARILOAD_DOPAIR;
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t*)&pack_load,sizeof(pack_load),0);
}

//*****************************************************************************
// 函数名称: AndonPair_Wakeup
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static inline void AndonPair_Wakeup(void)
{
    uint8_t pack_load;
    pack_load = ADV_PARILOAD_WAKEUP;
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t*)&pack_load,sizeof(pack_load),0);
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t*)&pack_load,sizeof(pack_load),0);
}

//*****************************************************************************
// 函数名称: AndonPair_SendPaired
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static inline void AndonPair_SendPaired(wiced_bt_device_address_t dst_mac)
{
    mesh_btn_pairload_t pack_load;

    WICED_LOG_DEBUG("Send Config Cmd to %B \n",dst_mac);
    pack_load.pair_stata = ADV_PARILOAD_PAIRED;
    memcpy(pack_load.dst_mac,dst_mac,sizeof(wiced_bt_device_address_t));
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t *)(&pack_load),sizeof(mesh_btn_pairload_t),0);
}

//*****************************************************************************
// 函数名称: AndonPair_SendPairSelect
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static inline void AndonPair_SendPairSelect(wiced_bt_device_address_t dst_mac)
{
    mesh_btn_pairload_t pack_load;

    WICED_LOG_DEBUG("Send Blink Cmd to %B \n",dst_mac);
    pack_load.pair_stata = ADV_PARILOAD_SELECT;
    memcpy(pack_load.dst_mac,dst_mac,sizeof(wiced_bt_device_address_t));
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t *)(&pack_load),sizeof(mesh_btn_pairload_t),1);
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Periodic_Timer_Callback(uint32_t param)
{
    if(WICED_FALSE == AndonPair_handle.AndonPair_doing)
    {
        return;
    }
    // 超时的检测
    if(AndonPair_handle.AndonPair_timer_cnt > ANDONPAIR_PAIRTIMELENGTH)
    {
        AndonPair_Stop();
        //wiced_stop_timer(&AndonPair_handle.AndonPair_timer);
        if(NULL != AndonPair_handle.AndonPair_Done_cback)
        {
            AndonPair_handle.AndonPair_Done_cback(ANDONPAIR_PAIR_FAILED_TIMEOUT);
        }
        AndonPair_handle.AndonPair_doing = WICED_FALSE;
        return;
    }

    //3s发送一次配对请求
    if(0 == (AndonPair_handle.AndonPair_timer_cnt%3))  //启动后主动发送
    {
        AndonPair_SendPairStart();
    }
    AndonPair_handle.AndonPair_timer_cnt++;
}

//*****************************************************************************
// 函数名称: mesh_btn_adv_paircmd_handle
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Cmdhandle(wiced_bt_device_address_t remote_mac, uint16_t cmd, void *data,uint8_t len)
{
    wiced_bt_device_address_t own_bd_addr;
    mesh_btn_pairload_t *p_adv_data = data;

    uint16_t i = 0;

    //TODO 接收绑定确认的处理
    
    //一次进进行一个设备的配对 遥控器仅能收到配对回复 收到的指令内容不正确不进行处理
    // WICED_LOG_DEBUG("Receive pair response form %B \r\n",remote_mac);
    // WICED_LOG_DEBUG("AndonPair_handle.AndonPair_doing = 0x%02x \r\n",AndonPair_handle.AndonPair_doing);
    if((WICED_TRUE != AndonPair_handle.AndonPair_doing ) || (cmd != ADV_CMD_PARI))
        return;
    
    //获取本地MAC
    //wiced_bt_dev_read_local_addr(own_bd_addr); 
    AppGetMyMac(own_bd_addr); 
    // WICED_LOG_DEBUG("receive from %B %B %B\r\n",remote_mac,p_adv_data->dst_mac,own_bd_addr);
    //由于使用的静态随机地址，同时使用了MAC的最高两位标识遥控器恢复出厂设置的次数，在比对前应将收取的MAC最高位或0xC0
    *(uint8_t *)(p_adv_data->dst_mac) |= 0xC0;
    *(uint8_t *)(own_bd_addr) |= 0xC0;
    //绑定的不是本遥控器
    if(0 != memcmp(p_adv_data->dst_mac,own_bd_addr,sizeof(wiced_bt_device_address_t)))
    {
        return;
    }

    if(ADV_PARILOAD_PAIREDACK == p_adv_data->pair_stata){
        //绑定完成
        if(0 == memcmp(paried_addr,remote_mac,sizeof(wiced_bt_device_address_t)))
        {
            if(NULL != AndonPair_handle.AndonPair_Done_cback)
            {
                AndonPair_handle.AndonPair_Done_cback(ANDONPAIR_PAIR_SUCCESS);
            }
        }
    }

    if(ADV_PARILOAD_PAIRACK != p_adv_data->pair_stata){
        return;
    }

    
    
    //是否已经存储在cache中
    for(i=0; i<AndonPair_Cache.max_pair_cacheIndex && i < ADV_PAIR_DEVICE_MAXNUM; i++)
    {
        if(0 == memcmp(remote_mac,AndonPair_Cache.pair_device_cache[i],sizeof(wiced_bt_device_address_t)))
        {
            return;
        }
    }
    WICED_LOG_DEBUG("receive from %B \r\n",remote_mac);
    memset(own_bd_addr,0,sizeof(wiced_bt_device_address_t));
    //查找使用的cache中是否有空闲位置
    for(i=0; i<AndonPair_Cache.max_pair_cacheIndex && i < ADV_PAIR_DEVICE_MAXNUM; i++)
    {
        if(0 == memcmp(AndonPair_Cache.pair_device_cache[i],own_bd_addr,sizeof(wiced_bt_device_address_t)))
        {
            memcpy(AndonPair_Cache.pair_device_cache[i],remote_mac,sizeof(wiced_bt_device_address_t));
            return;
        }
    }
    
    //缓冲区未满的存储当前mac，满的话则丢弃
    if(AndonPair_Cache.max_pair_cacheIndex < ADV_PAIR_DEVICE_MAXNUM)
    {
        memcpy(AndonPair_Cache.pair_device_cache[AndonPair_Cache.max_pair_cacheIndex],
               remote_mac, sizeof(wiced_bt_device_address_t));
        AndonPair_Cache.max_pair_cacheIndex ++;
        if(AndonPair_Cache.max_pair_cacheIndex > ADV_PAIR_DEVICE_MAXNUM)
        {
            AndonPair_Cache.max_pair_cacheIndex = ADV_PAIR_DEVICE_MAXNUM;
        }
    }
    else
    {
        AndonPair_Cache.max_pair_cacheIndex = ADV_PAIR_DEVICE_MAXNUM;
    }
    
    
    //如果是第一个，直接发送闪烁指令
    if(AndonPair_Cache.max_pair_cacheIndex == 1)
    {
        //发送闪烁指令
        AndonPair_handle.AndonPair_SelectLightFlag = WICED_TRUE;
        AndonPair_SendPairSelect(remote_mac);  
    }

    //AndonPair_SendPaired(remote_mac);
    //AndonPair_handle.AndonPair_doing = WICED_FALSE;
    //wiced_stop_timer(&AndonPair_handle.AndonPair_timer);
    //if(NULL != AndonPair_handle.AndonPair_Done_cback)
    //{
    //    AndonPair_handle.AndonPair_Done_cback(ANDONPAIR_PAIR_SUCCESS);
    //}
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Paired(void)
{
    uint16_t temp;
    wiced_bt_device_address_t empty_mac = {0};

    if((AndonPair_Cache.max_pair_cacheIndex == 0) || (AndonPair_handle.AndonPair_SelectLightFlag == WICED_FALSE))
    {
        return;
    }
    
    AndonPair_handle.AndonPair_SelectLightFlag = WICED_FALSE;
    AndonPair_handle.AndonPair_timer_cnt = 0;
    temp = (AndonPair_Cache.max_pair_cacheIndex<ADV_PAIR_DEVICE_MAXNUM)?AndonPair_Cache.max_pair_cacheIndex:ADV_PAIR_DEVICE_MAXNUM;
    if( AndonPair_Cache.current_pair_cacheIndex < temp ) 
    {
        //如果当前位置存储的MAC为空，不发送配对确认指令
        if(0 == memcmp(AndonPair_Cache.pair_device_cache[AndonPair_Cache.current_pair_cacheIndex],
                       empty_mac,sizeof(wiced_bt_device_address_t)))
        {
            return;
        }
        memcpy(paried_addr,
               AndonPair_Cache.pair_device_cache[AndonPair_Cache.current_pair_cacheIndex],
               sizeof(wiced_bt_device_address_t));
        AndonPair_SendPaired(paried_addr);
        WICED_LOG_DEBUG("confirm mac %B \r\n",paried_addr);
        memset(AndonPair_Cache.pair_device_cache[AndonPair_Cache.current_pair_cacheIndex],0,sizeof(wiced_bt_device_address_t));
    
        // if(NULL != AndonPair_handle.AndonPair_Done_cback)
        // {
        //     AndonPair_handle.AndonPair_Done_cback(ANDONPAIR_PAIR_SUCCESS);
        // }

    }
    else
    {
        WICED_LOG_ERROR("current_pair_cacheIndex Err!!!\n");
    }
    
}
//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Next(int16_t temp0)
{
    uint16_t temp,i;
    int16_t delta_num;
    wiced_bt_device_address_t empty_mac = {0};

    if(AndonPair_Cache.max_pair_cacheIndex == 0)
    {
        return;
    }
    
    AndonPair_handle.AndonPair_SelectLightFlag = WICED_TRUE;
    AndonPair_handle.AndonPair_timer_cnt = 0;
    temp = (AndonPair_Cache.max_pair_cacheIndex<ADV_PAIR_DEVICE_MAXNUM)?AndonPair_Cache.max_pair_cacheIndex:ADV_PAIR_DEVICE_MAXNUM;
    
    for(i=0; i<temp; i++)
    {
        //查找下一个不为空的位置
        if(temp0 > 0)
        {
            delta_num = (AndonPair_Cache.current_pair_cacheIndex+i+1)%temp;
        }
        else if(temp0 < 0)
        {
            delta_num = ((AndonPair_Cache.current_pair_cacheIndex+temp)-(i+1))%temp;
        }        
        //WICED_LOG_DEBUG("delta_num = %d\r\n",delta_num);
        if(0 != memcmp(AndonPair_Cache.pair_device_cache[delta_num],
                       empty_mac,sizeof(wiced_bt_device_address_t)))
        {
            WICED_LOG_DEBUG("temp = %d , i=%d , AndonPair_Cache.current_pair_cacheIndex=%d\r\n",
                            temp,i,AndonPair_Cache.current_pair_cacheIndex);
            AndonPair_Cache.current_pair_cacheIndex = delta_num;
            AndonPair_SendPairSelect(AndonPair_Cache.pair_device_cache[delta_num]);
            WICED_LOG_DEBUG("blink mac %B  index=%d\r\n",AndonPair_Cache.pair_device_cache[delta_num],delta_num);
            return;
        }
    }
    if(i == temp)
    {
        memset(&AndonPair_Cache,0,sizeof(mesh_paircache_t));
    }
    
}

//*****************************************************************************
// 函数名称: AndonPair_Stop
// 函数描述: 停止遥控器配对
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Stop(void)
{
    wiced_bt_device_address_t empty_mac = {0};
    AndonPair_handle.AndonPair_doing = WICED_FALSE;
    AndonPair_handle.AndonPair_SelectLightFlag = WICED_FALSE;
    AndonPair_SendPairSelect(empty_mac);
    WICED_LOG_DEBUG("blink mac %B  stop paired \r\n",empty_mac);
    wiced_stop_timer(&AndonPair_handle.AndonPair_timer);
    AndonPair_handle.AndonPair_timer_cnt = 0;
    memset(&AndonPair_Cache,0,sizeof(mesh_paircache_t));
}

static uint8_t rsttimer_tick = 0;
static wiced_timer_t Pair_Stoptimer;
//*****************************************************************************
// 函数名称: AndonPair_Stop
// 函数描述: 停止遥控器配对
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static void AndonPair_Stoptimer_callback(uint32_t arg)
{
    rsttimer_tick ++;
    if(1 == rsttimer_tick%15)
    {
        AndonPair_Stop();
    }
    if(rsttimer_tick == 40)
    {
        wiced_init_timer(&Pair_Stoptimer, &AndonPair_Stoptimer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
    }
    if(rsttimer_tick == 45)
    {
        wiced_stop_timer(&Pair_Stoptimer);
        wiced_hal_wdog_reset_system();
    }
}
//*****************************************************************************
// 函数名称: AndonPair_Start
// 函数描述: 启动遥控器配对
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Start(void)
{
    if(AndonPair_handle.AndonPair_Done_cback == NULL)
        return;
    if(WICED_TRUE == AndonPair_handle.AndonPair_doing)
        return;
    
    memset(&AndonPair_Cache,0,sizeof(mesh_paircache_t));
    AndonPair_handle.AndonPair_SelectLightFlag = WICED_FALSE;
    AndonPair_handle.AndonPair_doing = WICED_TRUE;
    wiced_start_timer(&AndonPair_handle.AndonPair_timer,1);
    AndonPair_handle.AndonPair_timer_cnt = 0;   
    //AndonPair_SendPairStart();           //在定时器中延迟1s启动查找灯指令
}

//*****************************************************************************
// 函数名称: AndonPair_Unband
// 函数描述: 设备解绑
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Unband(void)
{
    extern wiced_bool_t mesh_app_node_is_provisioned(void);

    uint8_t pack_load[2];
    uint8_t len = 0;

    // pack_load[0] = 0xFF;
    // pack_load[1] = 0xFF;
    if(!mesh_app_node_is_provisioned())
    {
        adv_vendor_send_cmd(ADV_CMD_RESET,pack_load,len,ADV_PACK_TTL);
    }
    else
    {
        adv_vendor_send_cmd(ADV_CMD_RESET,pack_load,len,ADV_PACK_TTL);
        //mesh_vendor_send_cmd(ADV_CMD_RESET,pack_load,len);
    }
}

//*****************************************************************************
// 函数名称: AndonPair_ResetFactor1
// 函数描述: 使用遥控器将灯恢复工厂模式指令
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_ResetFactor1(void)
{
    uint8_t pack_load;
    pack_load = ADV_PARILOAD_RESET;
    adv_vendor_send_cmd(ADV_CMD_PARI,(uint8_t*)&pack_load,sizeof(pack_load),1);
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_Init(AndonPair_Done_cback_t *cback)
{
    extern void mesh_start_stop_scan_callback(wiced_bool_t start, wiced_bool_t is_scan_active);
    AndonPair_handle.AndonPair_doing = WICED_FALSE;
    AndonPair_handle.AndonPair_Done_cback = cback;
    if(wiced_is_timer_in_use(&AndonPair_handle.AndonPair_timer))
    {
        wiced_deinit_timer(&AndonPair_handle.AndonPair_timer);
    }
    memset(&AndonPair_Cache,0,sizeof(mesh_paircache_t));
    wiced_init_timer(&AndonPair_handle.AndonPair_timer, &AndonPair_Periodic_Timer_Callback, 0, WICED_SECONDS_PERIODIC_TIMER);
    //发送唤醒指令
    mesh_start_stop_scan_callback(WICED_TRUE,WICED_FALSE);
    AndonPair_Wakeup();
    AndonPair_Start();//直接发送启动指令
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonPair_DeInit(wiced_bool_t para)
{
    wiced_bt_device_address_t empty_mac = {0};
    //AndonPair_SendPairSelect(empty_mac);
    //WICED_LOG_DEBUG("blink mac %B  stop paired \r\n",empty_mac);

    // 更改为按下配网键启动配网，一旦启动选灯则不能停止 20.3.30
    // if(para == WICED_TRUE) 
    // {
    //     if(wiced_is_timer_in_use(&Pair_Stoptimer))
    //     {
    //         wiced_deinit_timer(&Pair_Stoptimer);
    //     }
    //     wiced_init_timer(&Pair_Stoptimer, &AndonPair_Stoptimer_callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
    //     if(WICED_TRUE == AndonPair_handle.AndonPair_doing )
    //     {
    //         rsttimer_tick = 0;
    //     }
    //     else
    //     {
    //         rsttimer_tick = 39;
    //     }
    //     wiced_start_timer(&Pair_Stoptimer,20);
    // }
    AndonPair_handle.AndonPair_doing = WICED_FALSE;
    AndonPair_handle.AndonPair_SelectLightFlag = WICED_FALSE;
    AndonPair_handle.AndonPair_Done_cback = NULL;
    memset(&AndonPair_Cache,0,sizeof(mesh_paircache_t));
    if(wiced_is_timer_in_use(&AndonPair_handle.AndonPair_timer))
    {
        wiced_deinit_timer(&AndonPair_handle.AndonPair_timer);
    }
}
