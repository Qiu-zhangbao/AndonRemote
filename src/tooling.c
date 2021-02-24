//******************************************************************************
//*
//* 文 件 名 : tooling.c
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
#include "stdint.h"
#include "wiced_bt_cfg.h"
#include "wiced_hal_nvram.h"
#include "adv_pack.h"
#include "AES.h"
#include "wiced_hal_nvram.h"

#include "log.h"
#include "tooling.h"
#include "Andon_App.h"
#include "display.h"
#include "wiced_memory.h"
#include "storage.h"
#include "app_config.h"
#include "adv_pack.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************

#define DEFAULT_BURNIN_TIME                     40     //单位min, 默认的老化时间

#define TOOLINGTIMECYCLE                        1      //单位s
#define TOOLINGIN_STEP1_TIMEOUT                 10/TOOLINGTIMECYCLE  
#define TOOLING_BURNIN_STEP2_TIMEOUT            60/TOOLINGTIMECYCLE 
#define TOOLING_BURNIN_STEP3_TIMEOUT            2400/TOOLINGTIMECYCLE 

//#define TOOLINGIN_POWER_TIMEOUT                 15/TOOLINGTIMECYCLE

#define TOOL_MAX_RSSICNT                        15
#define TOOL_MIN_RSSI                           -70

///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void tooling_task(void);

///*****************************************************************************
///*                         常量定义区
///*****************************************************************************
const uint8_t productCode[] = PRODUCT_CODE;
///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
static tooling_burnin_t tooling_burnin_data={
    .item = {
        .burningFlag      = TOOLING_BURNIN_IDLE,
        .burninMin        = 0,
        .burninMax        = DEFAULT_BURNIN_TIME
    }
};

wiced_timer_t toolingtimer;
uint16_t      tooling_cnt;
uint16_t      toolrssi[TOOL_MAX_RSSICNT] ={0};
uint16_t      toolrssicnt = 0;
uint16_t      toolafterflag = 0;
int16_t       toolrssiset = -80;
uint16_t      toolpowerflag = 0;

///*****************************************************************************
///*                         函数实现区
///*****************************************************************************    
void ToolingTimerCb(uint32_t params)
{
    tooling_task();
    if(tooling_cnt > 0)
        tooling_cnt --;
}

//*****************************************************************************
// 函数名称: ToolDIDWriteVerify
// 函数描述: 校验DID是否写入
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
wiced_bool_t ToolDIDWriteVerify(void)
{
    uint32_t *data;
    uint16_t data_len;
    wiced_bool_t result = WICED_TRUE;

    data = (uint32_t*)wiced_bt_get_buffer(FLASH_XXTEA_SN_LEN+1);
    memset(data,0,FLASH_XXTEA_SN_LEN+1);
    wiced_hal_wdog_restart();
    data_len = storage_read_sn((uint8_t *)data);
    LOG_DEBUG("SN:%s\n",data);
    if(0 != memcmp((uint8_t *)data,productCode,sizeof(productCode)-1))
    {
        result = WICED_FALSE;
    }
    while((data_len > 3) && (result == WICED_TRUE))
    {
        if((*data == 0x00000000) || (*data == 0xFFFFFFFF))
        {
            result = WICED_FALSE;
            break;
        }
        data_len -= 4;
    }
    wiced_bt_free_buffer(data);
    LOG_DEBUG("result:%d\n",result);
    return result;
    
}
//*****************************************************************************
// 函数名称: tooling_task
// 函数描述: 工装处理
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void tooling_task(void)
{
    //已经完成了老化测试
    if(TOOLING_BURNIN_DONE == tooling_burnin_data.item.burningFlag)
    {
        return;
    }

    switch(tooling_burnin_data.item.burningFlag)
    {
        case TOOLING_BURNIN_IDLE:
        {
            return;
        }
        case TOOLING_BURNIN_STEP1:
        {
            if(0 == tooling_cnt)
            {
                //0.25s/0.25s亮灭两次--1.5s/1.5s呼吸一次 一直循环下去
                LOG_VERBOSE("tooling burin step1 err!!!\n");
                tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP1_ERR;
                andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_BLEFAILED;
            }
            return;
        }
        case TOOLING_BURNIN_STEP1_ERR:
        {
            return;
        }
        case TOOLING_BURNIN_STEP2:
        {
            andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_BLEOK;
            return;
        }
    }
}

//*****************************************************************************
// 函数名称: tooling_burninTest 
// 函数描述: 工装识别及接收信号强度判断
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
uint8_t tooling_burninTest(wiced_bt_ble_scan_results_t *p_scan_result)
{
    static wiced_bt_device_address_t tool_mac={0};
    int16_t rssiNum = 0;
    uint16_t i,j;
    
    rssiNum = p_scan_result->rssi;
    //接收到信号强度大于-80dbm，记录工装MAC，后续仅接收此工装消息，以有效判别接收的信号强度
    if(0 == memcmp("\x00\x00\x00\x00\x00",tool_mac,sizeof(wiced_bt_device_address_t)))
    {
        //if(rssiNum > -95) 
        //仅有当设备的信号强度大于检测强度时，锁定工装的MAC
        if(rssiNum > toolrssiset)
        {
            LOG_VERBOSE("tooling mac: %B\n", p_scan_result->remote_bd_addr);
            memcpy(tool_mac, p_scan_result->remote_bd_addr,sizeof(wiced_bt_device_address_t));
        }
        else
        {
            return 0;
        }
    }
    //非工装的MAC，不做处理
    if(0 != memcmp(tool_mac,p_scan_result->remote_bd_addr,sizeof(wiced_bt_device_address_t)))
    {
        return 0;
    }
    toolrssi[toolrssicnt] = rssiNum;
    
    //正向查找
    for(i=0; i<toolrssicnt; i++)
    {
        if(rssiNum < toolrssi[i]) 
        {
            break;
        }
    }
    //反向查找
    for(j=toolrssicnt; j>i; j--)
    {
        toolrssi[j] = toolrssi[j-1];
    }
    toolrssi[i] = rssiNum;

    toolrssicnt++;
    if(toolrssicnt > (TOOL_MAX_RSSICNT-1))
    {
        //分别去除最大和最小的3个值
        for(i=3; i<(TOOL_MAX_RSSICNT-3); i++) 
        {
            rssiNum+=toolrssi[i];
        }
        rssiNum = rssiNum/(TOOL_MAX_RSSICNT-6);
        LOG_DEBUG("tooling rssi: %d\n",rssiNum);
        if(rssiNum >= toolrssiset)  
        {
            return 0xFF;
        }
        else
        {
            memset(tool_mac, 0, sizeof(wiced_bt_device_address_t));
        }
        toolrssicnt = 0;
    }
    return 0;
}

//*****************************************************************************
// 函数名称: tooling_init
// 函数描述: 工装处理初始化
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void tooling_init(void)
{
    wiced_result_t ret;
    
    if(WICED_FALSE == ToolDIDWriteVerify())
    {
        andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_DIDFAILED;
        tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP1_ERR;
        return;
    }

    toolafterflag = 0;
    //此处从NVRAM中读取老化完成状态  TODO 分配工装标识的NVRAM ID
    //if(sizeof(tooling_burnin_t) != wiced_hal_read_nvram(NVRAM_TOOLING_POSITION, sizeof(tooling_burnin_t), tooling_burnin_data.burnin_array, &ret))
    {
        tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_IDLE;
        tooling_burnin_data.item.burninMin   = 0;
        tooling_burnin_data.item.burninMax   = DEFAULT_BURNIN_TIME;
        return;
    }
    // toolafterflag = 0;
    // //正在老化过程中，继续老化过程
    // if(tooling_burnin_data.item.burningFlag == TOOLING_BURNIN_STEP3)
    // {
    //     tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP2;
    //     if(wiced_is_timer_in_use(&toolingtimer))
    //     {
    //         wiced_deinit_timer(&toolingtimer);
    //     }
    //     wiced_init_timer(&toolingtimer,ToolingTimerCb,0,WICED_SECONDS_PERIODIC_TIMER);
    //     wiced_start_timer(&toolingtimer,1);
    //     //设置闪烁时间
    //     tooling_cnt = 5/TOOLINGTIMECYCLE;
    //     LOG_VERBOSE("tooling burin step3 continue： runtime = %d min\n", tooling_burnin_data.item.burninMin);
    //     //set flash 0.5s/0.5s
    //     LightFlash(100,1000);
    // }
    // else if(tooling_burnin_data.item.burningFlag != TOOLING_BURNIN_DONE)
    // {
    //     tooling_burnin_data.item.burningFlag  = TOOLING_BURNIN_IDLE;
    //     tooling_burnin_data.item.burninMin   = 0;
    //     tooling_burnin_data.item.burninMax   = DEFAULT_BURNIN_TIME;
    // }
}



//*****************************************************************************
// 函数名称: tooling_handle
// 函数描述: 接收工装数据的处理
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void tooling_handle(wiced_bt_ble_scan_results_t *p_scan_result,uint16_t cmd, uint8_t *data, uint8_t len)
{
    if(cmd != ADV_CMD_TOOLING)  //非工装指令，直接返回
        return;


    switch(data[0])
    {
        case ADV_TOOLING_BURNIN:
            
            if(andon_app_state.run_mode != ANDON_APP_RUN_TEST)
            {
                return;
            }
            switch(tooling_burnin_data.item.burningFlag)
            {
                case TOOLING_BURNIN_IDLE:
                    if((toolafterflag != 0) || (0 != toolpowerflag))
                    {
                        return;
                    }
                    if(wiced_is_timer_in_use(&toolingtimer))
                    {
                        wiced_deinit_timer(&toolingtimer);
                    }
                    LOG_VERBOSE("tooling burn_in start\n");
                    {
                        uint8_t recproduct[8] = {0};
                        memcpy(recproduct,data+2,sizeof(productCode)-1-3);
                        LOG_DEBUG("recproduct %s\n",recproduct);
                    }
                    if(0 == memcmp((uint8_t *)(data+2),productCode+3,sizeof(productCode)-1-3))
                    {
                        wiced_bool_t AndonApppaired1;
                        AndonApppaired1 = WICED_FALSE;
                        flash_write_erase(FLASH_ADDR_STATE, (uint8_t*)(&AndonApppaired1), sizeof(AndonApppaired1),WICED_TRUE);
                        advpackResetRemoteIndex();
                        StoreBindKey(NULL,0);
                        toolrssiset = (int8_t)data[1];
                        tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP1;
                        wiced_init_timer(&toolingtimer,ToolingTimerCb,0,WICED_SECONDS_PERIODIC_TIMER);
                        wiced_start_timer(&toolingtimer,TOOLINGTIMECYCLE);
                        tooling_burninTest(p_scan_result);
                        tooling_cnt = TOOLINGIN_STEP1_TIMEOUT;
                    }
                    // else
                    // {
                    //     tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP1_ERR;
                    //     andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_MODEFAILED;
                    // }
                break;
                case TOOLING_BURNIN_STEP1:
                    if(0xFF == tooling_burninTest(p_scan_result))
                    {
                        LOG_VERBOSE("tooling burin step1 done\n");
                        if(WICED_FALSE == ToolDIDWriteVerify())
                        {
                            tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP1_ERR;
                            andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_DIDFAILED;
                        }
                        else
                        {
                            tooling_burnin_data.item.burningFlag = TOOLING_BURNIN_STEP2;
                        }
                    }
                break;
                default:
                break;
            }
        break;
         case ADV_TOOLING_UPGRADE_VER:  //工厂升级时的版本信息
            LOG_DEBUG("11111111..........................\n");
            //不是本型号机型适配的工装
            if(0 != memcmp((uint8_t *)(data+3),productCode+3,sizeof(productCode)-1-3)){
                return;
            }

            uint16_t toolfwver;

            toolfwver = data[1];
            toolfwver = toolfwver*256 + data[2];
            if(toolfwver == MESH_FWID){  //版本相同，禁止ADV
                appSetConnAdvDisable();
                andon_app_state.run_mode = ANDON_APP_RUN_TEST;
                andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_UPGRADEOK;
            }
            else{   //版本不同, 使能ADV广播,
                appSetConnAdvEnable();
                andon_app_state.run_mode = ANDON_APP_RUN_TEST;
                andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_DOING;
            }
        break;
        default:
        break;
    }
}

//*****************************************************************************
// 函数名称: toolSmtCheck
// 函数描述: SMT联通性检测
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void toolSmtCheck(void)
{
#include "wiced_rtos.h"
#include "wiced_hal_wdog.h"


    //设置除触发按键之外的GPIO为输出
    wiced_hal_gpio_select_function(WICED_P28,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P28, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
    wiced_hal_gpio_select_function(WICED_P29,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P29, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    wiced_hal_gpio_select_function(WICED_P26,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P26, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
    wiced_hal_gpio_select_function(WICED_P27,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P27, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    wiced_hal_gpio_select_function(WICED_P01,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P01, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    wiced_hal_gpio_select_function(WICED_P00,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P00, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
    wiced_hal_gpio_select_function(WICED_P08,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P08, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    wiced_hal_gpio_select_function(WICED_P03,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P03, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_HIGH);
    wiced_hal_gpio_select_function(WICED_P02,WICED_GPIO);
    wiced_hal_gpio_configure_pin(WICED_P02, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    
    if(WICED_FALSE == ToolDIDWriteVerify())
    {
        wiced_hal_wdog_disable();
        while(1);
    }
    while(1)
    {
        wiced_hal_wdog_restart();
        wiced_rtos_delay_milliseconds(600, KEEP_THREAD_ACTIVE);
        wiced_hal_gpio_set_pin_output(WICED_P28,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P29,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P26,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P27,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P01,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P00,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P08,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P03,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P02,GPIO_PIN_OUTPUT_HIGH);
        wiced_rtos_delay_milliseconds(600, KEEP_THREAD_ACTIVE);
        wiced_hal_gpio_set_pin_output(WICED_P28,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P29,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P26,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P27,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P01,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P00,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P08,GPIO_PIN_OUTPUT_LOW);
        wiced_hal_gpio_set_pin_output(WICED_P03,GPIO_PIN_OUTPUT_HIGH);
        wiced_hal_gpio_set_pin_output(WICED_P02,GPIO_PIN_OUTPUT_LOW);
    } 

}

