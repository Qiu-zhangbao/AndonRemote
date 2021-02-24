/*
 * Copyright 2018, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor 
 *  Corporation. All rights reserved. This software, including source code, documentation and  related 
 * materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its 
 *  subsidiaries ("Cypress") and is protected by and subject to worldwide patent protection  
 * (United States and foreign), United States copyright laws and international treaty provisions. 
 * Therefore, you may use this Software only as provided in the license agreement accompanying the 
 * software package from which you obtained this Software ("EULA"). If no EULA applies, Cypress 
 * hereby grants you a personal, nonexclusive, non-transferable license to  copy, modify, and 
 * compile the Software source code solely for use in connection with Cypress's  integrated circuit 
 * products. Any reproduction, modification, translation, compilation,  or representation of this 
 * Software except as specified above is prohibited without the express written permission of 
 * Cypress. Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO  WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING,  BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to 
 * the Software without notice. Cypress does not assume any liability arising out of the application 
 * or use of the Software or any product or circuit  described in the Software. Cypress does 
 * not authorize its products for use in any products where a malfunction or failure of the 
 * Cypress product may reasonably be expected to result  in significant property damage, injury 
 * or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the 
 *  manufacturer of such system or application assumes  all risk of such use and in doing so agrees 
 * to indemnify Cypress against all liability.
 */

/******************************************************************************
 *                                Includes
 ******************************************************************************/

#include "sparcommon.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_trace.h"
#include "wiced_gki.h"
#include "wiced_platform.h"
#include "wiced_hal_puart.h"
#include "wiced_timer.h"
#include "wiced_bt_stack.h"
#include "raw_flash.h"
#include "log.h"


//#define APPLICATION_SPECIFIC_FLASH_RESERVATION 2

uint32_t flash_app_data_store_addr_base = 0;

void flash_app_init(void)
{
// #if !USE_NVRAM
//     uint32_t total_flash_size = FLASH_SIZE;

//     wiced_hal_sflash_init();

//     wiced_hal_sflash_set_size(total_flash_size);

//     flash_app_data_store_addr_base = (total_flash_size - (APPLICATION_SPECIFIC_FLASH_RESERVATION * FLASH_SECTOR_SIZE));
// #endif
}

//Reads the given length of data from SF/EEPROM. If success returns len, else returns 0
uint32_t flash_app_read_mem(uint32_t read_from, uint8_t *buf, uint32_t len)
{
    int retry = 0;
    while (retry < 5)
    {
        LOG_DEBUG("Read start\n");
        if (wiced_hal_sflash_read(read_from, len, buf) == len)
        {
            LOG_DEBUG("Read OK, times: %d\n", retry);
            return len;
        }
        retry++;
    }

    LOG_ERROR("Read failure.\n");
    return 0;
}

uint32_t flash_write_erase(uint32_t addr, uint8_t *data, uint32_t len,uint8_t erase)
{
    if(erase == WICED_TRUE)
    {
        LOG_DEBUG("erase start\n");
        wiced_hal_sflash_erase(addr&0xFFFFF000, 4096);
        LOG_DEBUG("erase end\n");
    }

    int retry = 0;
    while (retry < 5)
    {
        LOG_DEBUG("write start\n");
        if (wiced_hal_sflash_write(addr, len, data) == len)
        {
            LOG_DEBUG("Write OK, times: %d\n", retry);
            return len;
        }
        LOG_DEBUG("erase start\n");
        wiced_hal_sflash_erase(addr&0xFFFFF000, 4096);
        LOG_DEBUG("erase end\n");
        retry++;
    }

    LOG_ERROR("Write failure.\n");
    return 0;
}


uint32_t flash_app_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    return flash_app_read_mem(addr, data, len);
}



