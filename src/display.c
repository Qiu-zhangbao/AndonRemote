//******************************************************************************
//*
//* 文 件 名 : 
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
#include "wiced_bt_trace.h"
#include "display.h"
#include "Andon_App.h"
#include "mesh_btn_handler.h"
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

///*****************************************************************************
///*                         常量定义区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
static uint16_t display_cnt;
static uint16_t display_flash_cycle = 0;
static uint16_t display_flash_period = 0;
///*****************************************************************************
///*                         函数实现区
///*****************************************************************************

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void DisplayRefresh(wiced_bool_t is_provison)
{
    static uint16_t last_mode = ANDON_APP_DISPLAY_NORMAL;
    
    display_cnt++;

    if(last_mode != andon_app_state.display_mode)
    {
        last_mode = andon_app_state.display_mode;
        display_cnt = 0;
    }
    
    // if((andon_app_state.display_mode != ANDON_APP_DISPLAY_NORMAL) 
    //     && (andon_app_state.display_mode != ANDON_APP_DISPLAY_CONFIG))
    // {
    //     if(display_cnt % 25 == 1)
    //     {
    //         WICED_BT_TRACE("andon_app_state.display_mode: %d\n",andon_app_state.display_mode);
    //         WICED_BT_TRACE("btn_state.btn_press_count[BTN_RESET_INDEX]: %d\n",btn_state.btn_press_count[BTN_RESET_INDEX]);
    //         WICED_BT_TRACE("btn_state.btn_encoder_count: %d\n",btn_state.btn_encoder_count);
    //     }
    // }
    
    switch(andon_app_state.display_mode)
    {
        case ANDON_APP_DISPLAY_NORMAL:
        case ANDON_APP_DISPLAY_TEST_BLEOK:
        {
            // if(is_provison)
            // {
            //     LEDB_ON;
            //     LEDY_OFF;
            // }
            // else
            // {
            //     LEDY_ON;
            //     LEDB_OFF;
            // }
            LEDB_OFF;
            LEDY_OFF;
            display_cnt = 0;
            break;
        }
        
        case ANDON_APP_DISPLAY_CONFIG:
        {
            if(is_provison)
            {
                LEDB_ON;
                LEDY_OFF;
            }
            else
            {
                LEDY_ON;
                LEDB_OFF;
            }
            display_cnt = 0;
            break;
        }
        case ANDON_APP_DISPLAY_BTN_RESET3S:
        {
            if(is_provison)
            {
                LEDB_ON;
                LEDY_OFF;
            }
            else
            {
                LEDY_ON;
                LEDB_OFF;
            }
            break;
        }
        case ANDON_APP_DISPLAY_BTN_PRESS_NORMAL:
        {
            if(display_cnt == 1)
            {
                if(is_provison)
                {
                    LEDB_ON;
                }
                else
                {
                    LEDY_ON;
                }
            }
            else if(display_cnt == 100/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(200/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_BTN_RESET10S:
        case ANDON_APP_DISPLAY_BTN_PRESS_CONFIG:
        {
            if(display_cnt == 1)
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            else if(display_cnt == 100/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                if(is_provison)
                {
                    LEDB_ON;
                }
                else
                {
                    LEDY_ON;
                }
            }
            display_cnt = display_cnt%(200/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_PAIR_DOING:
        {
            if((1 == display_cnt) || (display_cnt == 400/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                if(is_provison)
                {
                    LEDB_ON;
                }
                else
                {
                    LEDY_ON;
                }
            }
            else if( (display_cnt == 200/ANDON_APP_PERIODIC_TIME_LENGTH) 
                     || (display_cnt == 600/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDY_OFF;
                LEDB_OFF;
            }
            else if(display_cnt == 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                display_cnt = 0;
            }
            break;
        }
        case ANDON_APP_DISPLAY_PAIR_SUCCESS:
        {
            //TODO 常亮1s 更改显示模式为常规模式
            LEDY_ON;
            LEDB_OFF;
            break;
        }
        case ANDON_APP_DISPLAY_PAIR_FAILED:
        {
            //TODO 暂不处理，将显示模式更改为常规模式
            //andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            LEDY_ON;
            LEDB_OFF;
            break;
        }
        case ANDON_APP_DISPLAY_PROVISION_DOING:
        {
            if((display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH)) == 1)
            {
                LEDY_ON;
                LEDB_OFF;
            }
            else if(display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH) == (320/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_ON;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_PROVISION_DONE:
        {
            if(display_cnt >= (3000/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                LEDB_ON;
                LEDY_OFF;
                display_cnt = 0;
                return;
            }
            if(display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH) == 1)
            {
                LEDY_ON;
                LEDB_OFF;
            }
            else if(display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH) == (320/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_ON;
                LEDY_OFF;
            }
            break;
        }
        case ANDON_APP_DISPLAY_FLASH:
        {
            if((display_cnt > display_flash_period) || (display_flash_period==0) || (display_flash_cycle==0))
            {
                display_flash_period = 0;
                display_flash_cycle = 0;
                display_cnt = 0;
                andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                return;
            }

            if(display_cnt%display_flash_cycle == 1)
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            else if(display_cnt%display_flash_cycle == display_flash_cycle/2)
            {
                if(is_provison)
                {
                    LEDB_ON;
                }
                else
                {
                    LEDY_ON;
                }
            }
            break;
        }
        case ANDON_APP_DISPLAY_LOWPOWER:
        {
            // if(is_provison)
            // {
            //     LEDB_ON;
            // }
            // else
            // {
            //     LEDY_ON;
            // }
            if(display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH) == 1)
            {
                LEDY_ON;
                LEDB_OFF;
            }
            else if(display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH) == (100/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_TEST_DOING:
        {
            LEDB_ON;
            LEDY_OFF;
            break;
        }
        case ANDON_APP_DISPLAY_TEST_UPGRADEOK:
        {
            if(display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH) == 1)
            {
                LEDB_ON;
                LEDY_OFF;
            }
            else if(display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH) == (100/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(1000/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_TEST_BLEFAILED:
        {
            if(display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH) == 1)
            {
                LEDY_ON;
                LEDB_OFF;
            }
            else if(display_cnt%(640/ANDON_APP_PERIODIC_TIME_LENGTH) == (320/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_ON;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(1280/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
            
        }
        case ANDON_APP_DISPLAY_TEST_MODEFAILED:
        {
            if((display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH)== 1)
              || (display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (200/ANDON_APP_PERIODIC_TIME_LENGTH)) )
            {
                LEDB_ON;
                LEDY_OFF;
            }
            else if((display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (100/ANDON_APP_PERIODIC_TIME_LENGTH))
                   || (display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (300/ANDON_APP_PERIODIC_TIME_LENGTH)) )
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            else if(display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (400/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_OFF;
                LEDY_ON;
            }
            else if(display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (500/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        case ANDON_APP_DISPLAY_TEST_DIDFAILED:
        {
            if((display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH)== 1)
              || (display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (200/ANDON_APP_PERIODIC_TIME_LENGTH)) )
            {
                LEDY_ON;
                LEDB_OFF;
            }
            else if((display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (100/ANDON_APP_PERIODIC_TIME_LENGTH))
                   || (display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (300/ANDON_APP_PERIODIC_TIME_LENGTH)) )
            {
                LEDY_OFF;
                LEDB_OFF;
            }
            else if(display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (400/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDY_OFF;
                LEDB_ON;
            }
            else if(display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH) == (500/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                LEDB_OFF;
                LEDY_OFF;
            }
            display_cnt = display_cnt%(3000/ANDON_APP_PERIODIC_TIME_LENGTH);
            break;
        }
        default:
        {
            display_cnt = 0;
            andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            break;
        }
    }
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void LightFlash(uint16_t cycle, uint16_t times)
{
    display_flash_cycle = cycle*10/20;
    display_flash_period = display_flash_cycle*times;
    andon_app_state.display_mode = ANDON_APP_DISPLAY_FLASH;
    display_cnt = 0;
}
//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void DisplayInit(void)
{
    display_cnt = 0;
    LEDY_OFF;
    LEDB_OFF;
}

//*****************************************************************************
// 函数名称: 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void DisplayDeinit(void)
{
    display_cnt = 0;
    LEDY_OFF;
    LEDB_OFF;
}