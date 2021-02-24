//******************************************************************************
//*
//* �� �� �� : Andon_App.c
//*   MCU    : CYW920735/WICED       
//* �ļ����� : ����mesh switchӦ���߼�����       
//* ��    �� : ����/Andon Health CO.Ltd
//* ��    �� : V0.0
//* ��    �� : 
//*
//* ������ʷ : 
//*     ����       ����    �汾     ����
//*         
//*                   
//******************************************************************************

///*****************************************************************************
///*                         �����ļ�˵��
///*****************************************************************************
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_cfg.h"
#include "wiced_hal_nvram.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_bt_mesh_provision.h"
#include "wiced_bt_btn_handler.h"
#include "wiced_bt_mesh_core_extra.h"
#include "wiced_bt_mesh_lpn.h"
#include "mesh_btn_handler.h"
#include "Andon_App.h"
#include "AndonPair.h"
#include "app_config.h"
#include "adv_pack.h"
#include "AndonCmd.h"
#include "wiced.h"
#include "display.h"
#include "includes.h"
#include "wiced_hal_adc.h"
#include "mesh_application.h"
#include "tooling.h"
#include "clock_timer.h"
#include "storage.h"
#include "Beep.h"
#include "mylib.h"

#define TAG  "Andon_App"

#define WICED_LOG_LEVEL   WICDE_DEBUG_LEVEL

///*****************************************************************************
///*                         �궨����
///*****************************************************************************
#define MESH_REMOTE_CLIENT_ELEMENT_INDEX  0
#define ANDON_APP_UNPROVISON              0    //δ����
#define ANDON_APP_APP_PROVISON            1    //APP����
#define ANDON_APP_SELF_PROVISON           2    //ң��������

#define USE_EXTRA_API                    // Use extra mesh_core APIs to access some parameters

#define ANDONAPP_DELTACMD_INTERVAL        400    //��λΪms

#if CHECK_BATTERY_VALUE
#define CHANNEL_TO_MEASURE_DC_VOLT        ADC_INPUT_P29
#endif
///*****************************************************************************
///*                         Strcut�ṹ����������
///*****************************************************************************
typedef struct 
{
    uint16_t send_delta_timecnt;              //���ڷ���deltaָ���ʱ
    uint16_t send_onoff_timecnt;              //���ڷ���onoffָ���ʱ  Ϊ0ʱ��ʾδ�������ͼ�ʱ
    uint16_t btn_fastpress_numcnt;            //���ڼ�¼���ٰ�ONOFF�����Ĵ���
    uint16_t btn_onoff_lasttime;              //���ڼ�¼����һ��ONOFF������ʱ��
    uint16_t btn_encoder_numcnt;              //���ڼ�¼�����������Ŀ�ʼʱ��
    UINT64   btn_encoder_start_time;          //���ڼ�¼�����������Ŀ�ʼʱ��
    UINT64   btn_encoder_end_time;            //���ڼ�¼�����������Ľ���ʱ��
    wiced_bool_t btn_onoff_cmdstata;          //���ڼ�¼����һ��ONOFFָ���״̬--ON or OFF
}andon_app_static_handle_t;



///*****************************************************************************
///*                         ����������
///*****************************************************************************
static void Andon_App_Provision_Stata_Change(uint16_t event, void *p_data);
static void Andon_App_Periodic_Timer_Callback(uint32_t arg);

//extern void mesh_onoff_btn_event_handler(void);
//extern void mesh_level_reduce(void);
//extern void mesh_level_plus(void);
extern void mesh_application_factory_reset(void);
extern void mesh_interrupt_handler(void* user_data, uint8_t pin);
extern void low_power_set_allow_sleep_flag(uint8_t allow_sleep);

///*****************************************************************************
///*                         ����������
///*****************************************************************************

///*****************************************************************************
///*                         �����ⲿȫ�ֱ���������
///*****************************************************************************
extern wiced_bt_mesh_provision_server_callback_t *mesh_app_provision_handler;
extern wiced_bt_cfg_settings_t wiced_bt_cfg_settings;

///*****************************************************************************
///*                         �ⲿȫ�ֱ���������
///*****************************************************************************
andon_app_handler_t andon_app_state;

extern UINT32 keyscan_stuck_key_in_second;
///*****************************************************************************
///*                         �ļ��ڲ�ȫ�ֱ���������
///*****************************************************************************
static uint16_t AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
static uint16_t AndonApp_power_stata;
static uint32_t AndonApp_runtime = 0;
static uint8_t  fastturn = 0;
static uint8_t  fastturnflag = 0;  //1 ��ʾ˳ʱ�� 2��ʾ��ʱ�� 0��ʾδʶ��
// static uint16_t AndonApp_proed_waitting = 0;
// static uint16_t AndonApp_proed_selectdev = 0;
static wiced_bool_t AndonApppaired = WICED_FALSE;
static andon_app_static_handle_t andon_app_staticstata;

#ifdef ANDON_BEEP
static uint32_t  AndonAppBeepTimer= 0;
#endif

// PLACE_IN_ALWAYS_ON_RAM wiced_bool_t onoff_stata = WICED_TRUE;

#if CHECK_BATTERY_VALUE
// #define ANDONBATTERYARRAYLENGTH   16
// static uint32_t andonBatteryArray[ANDONBATTERYARRAYLENGTH]={0};
// static uint32_t andonBatteryIndex = 0;
#endif

static wiced_bool_t appconnadvenable = WICED_FALSE;

extern uint8_t mesh_system_id[WICED_BT_MESH_PROPERTY_LEN_DEVICE_SERIAL_NUMBER];
extern uint8_t mesh_fW_ver[WICED_BT_MESH_PROPERTY_LEN_DEVICE_FIRMWARE_REVISION]; 
extern uint8_t mesh_hW_ver[WICED_BT_MESH_PROPERTY_LEN_DEVICE_HARDWARE_REVISION]; 

///*****************************************************************************
///*                         ����ʵ����
///*****************************************************************************

//*****************************************************************************
// ��������: mesh_remote_lpn_friendship
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
#if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
static void mesh_remote_lpn_friendship(wiced_bool_t established, uint16_t frined_node_id)
{
    WICED_LOG_DEBUG("%s: established = %d, friend ID = 0x%x\n", __func__, established, frined_node_id);
}

//*****************************************************************************
// ��������: mesh_remote_frnd_friendship
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
#elif LOW_POWER_NODE == MESH_LOW_POWER_NODE_FRIEND
static void mesh_remote_frnd_friendship(wiced_bool_t established, uint16_t lpn_node_id)
{
    WICED_LOG_DEBUG("%s: established = %d, LPN ID = 0x%x\n", __func__, established, lpn_node_id);
}

#endif


#if MESH_DFU
static wiced_bool_t mesh_provisioner_validate_fw_data(uint8_t *p_data, uint16_t data_len)
{
    wiced_bool_t result = WICED_TRUE;
    uint8_t validation_data_len;
    uint16_t company_id, mesh_pid, mesh_vid;
    const char *str = "validate_fw_data: ";

    LOG_DEBUG("%sdata_len:%d\n", str, data_len);

    // Input data format: validation data length (1 byte), validation data, firmware ID

    STREAM_TO_UINT8(validation_data_len, p_data);

    // To be implemented: check validation data

    // Check firmware ID
    p_data += validation_data_len;
    data_len -= 1 + validation_data_len;
    if (data_len < 6)
        return WICED_FALSE;
    BE_STREAM_TO_UINT16(company_id, p_data);
    BE_STREAM_TO_UINT16(mesh_pid, p_data);
    BE_STREAM_TO_UINT16(mesh_vid, p_data);
    if (company_id != mesh_config.company_id || mesh_pid != mesh_config.product_id || mesh_vid != mesh_config.vendor_id)
    {
        LOG_DEBUG("%sOops - CID:%04X, PID:%04X, VID:%04X\n", str, company_id, mesh_pid, mesh_vid);
        result = WICED_FALSE;
    }
    return result;
}

static wiced_bool_t mesh_provisioner_fw_update_server_callback(uint16_t event, uint8_t *p_data, uint16_t data_len)
{
    wiced_bool_t result = WICED_TRUE;

    LOG_DEBUG("fw_update_cb: event:%d, data_len:%d\n", event, data_len);
    switch (event)
    {
    case WICED_BT_MESH_FW_UPDATE_VALIDATE:
        result = mesh_provisioner_validate_fw_data(p_data, data_len);
        break;
    case WICED_BT_MESH_FW_UPDATE_APPLY:
        // Clean up before switching to new FW
        break;
    default:
        break;
    }
    return result;
}


#endif

//*****************************************************************************
// ��������: andon_app_provision_stata_change
// ��������: provision״̬�ı�Ļص�
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
static void Andon_App_Provision_Stata_Change(uint16_t event, void *p_data)
{
    switch(event)
    {
    case WICED_BT_MESH_PROVISION_STARTED:
    {
        andon_app_state.display_mode = ANDON_APP_DISPLAY_PROVISION_DOING;
        //WICED_LOG_DEBUG("Btn provision start \r\n");
        break; 
    }
    case WICED_BT_MESH_PROVISION_END:
    {
        andon_app_state.display_mode = ANDON_APP_DISPLAY_PROVISION_DONE;
        andon_app_state.is_provison = WICED_TRUE;
        //WICED_LOG_DEBUG("Btn provision end \r\n");
        break;
    }
    }
}

//*****************************************************************************
// ��������: 
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
void Andon_App_AdvPairDone(uint8_t result)
{
    // LOG_DEBUG("0.................... result:%d\n",result);
    if(ANDONPAIR_PAIR_SUCCESS == result)
    {
        // LOG_DEBUG("1.................... AndonApppaired:%d\n",AndonApppaired);
        // AndonApp_pair_stata = ANDONPAIR_PAIR_DONE;
        if(AndonApppaired == WICED_FALSE){
            AndonApppaired = WICED_TRUE;
            // LOG_DEBUG("2....................\n");
            flash_write_erase(FLASH_ADDR_STATE, (uint8_t*)(&AndonApppaired), sizeof(AndonApppaired),WICED_TRUE);
        }
        // andon_app_state.display_mode = ANDON_APP_DISPLAY_PAIR_SUCCESS ;
    }
    else if(ANDONPAIR_PAIR_FAILED_TIMEOUT == result)  
    {
        // AndonApp_pair_stata = ANDONPAIR_PAIR_FAILED;
        // andon_app_state.display_mode = ANDON_APP_DISPLAY_PAIR_FAILED;
    }
}

//*****************************************************************************
// ��������: 
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
void Andon_App_SleepSet(void)
{
    extern void mesh_start_stop_scan_callback(wiced_bool_t start, wiced_bool_t is_scan_active);

    WICED_LOG_DEBUG("%s callback\n",__func__);
    if(AndonApp_power_stata != 0xFFFF)
    {
        AndonApp_power_stata = 0xFFFF;
        WICED_LOG_VERBOSE("enable keyscan\n");
        AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
        AndonPair_DeInit(WICED_FALSE);
        #ifdef ANDON_BEEP
        AnondBeepOff();
        #endif
    //     wiced_platform_register_button_callback(WICED_PLATFORM_BUTTON_1, mesh_interrupt_handler, NULL, GPIO_INTERRUPT_DISABLE);
    //     wiced_platform_register_button_callback(WICED_PLATFORM_BUTTON_2, mesh_interrupt_handler, NULL, GPIO_INTERRUPT_DISABLE);
    //     wiced_platform_register_button_callback(WICED_PLATFORM_BUTTON_3, mesh_interrupt_handler, NULL, GPIO_INTERRUPT_DISABLE);
    }
    else 
    {
        extern void wiced_hal_keyscan_reset(void);
        WICED_LOG_VERBOSE("stop timer\n");
        LEDY_OFF;
        LEDB_OFF;
        // ����ң������Թ���
        // example_key_init();
        mesh_start_stop_scan_callback(WICED_FALSE,WICED_FALSE);
        wiced_stop_timer(&andon_app_state.periodic_timer);
        //wiced_hal_keyscan_reset();
    }
}


void appStartDevAdv(void)
{
    // uint8_t devdata[16] = {0,0xFF,0x70,0x08,0x03,0x07};

    // wiced_bt_dev_read_local_addr(devdata+6);
    // // if(mesh_app_node_is_provisioned())  //�����������������Զ���㲥
    // if(WICED_TRUE == storageBindkey.bindflag)
    // {
    //     devdata[12] = 0x01;
    //     // return;
    // }
    // else
    // {
    //     devdata[12] = 0x00;
    // }
    // devdata[13] = 0;
    // //memcpy(devdata+4,PRODUCT_CODE,sizeof(PRODUCT_CODE)-1);
    // devdata[0] = 5 + sizeof(wiced_bt_device_address_t) + 1 + 1;
    // adv_manuDevAdvStart(devdata,devdata[0]+1);
}

void appBleConnectNotify(wiced_bool_t isconneted)
{
    void AndonServiceGattDisConnect(void);

    if(isconneted == WICED_FALSE)
    {
        //appStartDevAdv();
        AndonServiceGattDisConnect();
        appAndonBleConnectUsed();
    }
    else
    {
        //adv_manuDevAdvStop();
        if(AndonApp_pair_stata != ANDONPAIR_PAIR_IDLE)
        {
            AndonPair_Stop();
            andon_app_state.run_mode = ANDON_APP_RUN_NORMAL;
            andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            //����ң������Թ���
            AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
            AndonPair_DeInit(WICED_TRUE);
            btn_state.btn_idle_count = 0;
            btn_state.btn_encoder_count = 0;
        }
    }
    
}

void AndonAppSetDevInfo(void)
{
#if RELEASE
    static int8_t             dev_name[22] = PRODUCT_CODE;
#else
    static int8_t             dev_name[22] = "Switch "; 
#endif

#if ANDON_TEST   
    memcpy(dev_name+strlen(dev_name),__DATE__,sizeof(__DATE__));
#endif

    {
        uint8_t len=0,temp=0;
        temp = (uint8_t)(((MESH_FWID>>8)&0xF0)>>4);
        if(temp > 9)
        {
            temp = 9;
        }
        mesh_fW_ver[len++] = temp + 0x30;
        mesh_fW_ver[len++] = '.';
        temp = (uint8_t)((MESH_FWID>>8)&0x0f);
        if(temp > 9)
        {
            temp = 9;
        }
        mesh_fW_ver[len++] = temp + 0x30;
        mesh_fW_ver[len++] = '.';
        temp = (uint8_t)(MESH_FWID&0xFF);
       if(temp > 99){
            mesh_fW_ver[len++] = temp/100 + 0x30;
            mesh_fW_ver[len++] = (temp%100)/10 + 0x30;
            mesh_fW_ver[len++] = temp%10 + 0x30;
        }else if(temp > 9){
            mesh_fW_ver[len++] = temp/10 + 0x30;
            mesh_fW_ver[len++] = temp%10 + 0x30;
        }else{
            mesh_fW_ver[len++] = temp + 0x30;
        }

        len = 0;
        temp = (uint8_t)(((MESH_VID>>8)&0xF0)>>4);
        if(temp > 9)
        {
            temp = 9;
        }
        mesh_hW_ver[len++] = temp + 0x30;
        mesh_hW_ver[len++] = '.';
        temp = (uint8_t)((MESH_VID>>8)&0x0f);
        if(temp > 9)
        {
            temp = 9;
        }
        mesh_hW_ver[len++] = temp + 0x30;
        mesh_hW_ver[len++] = '.';
        temp = (uint8_t)(MESH_VID&0xFF);
        if(temp > 99){
            mesh_hW_ver[len++] = temp/100 + 0x30;
            mesh_hW_ver[len++] = (temp%100)/10 + 0x30;
            mesh_hW_ver[len++] = temp%10 + 0x30;
        }else if(temp > 9){
            mesh_hW_ver[len++] = temp/10 + 0x30;
            mesh_hW_ver[len++] = temp%10 + 0x30;
        }else{
            mesh_hW_ver[len++] = temp + 0x30;
        }
    }

    {
        uint8_t *pdata;
        pdata = (uint8_t *)wiced_bt_get_buffer(FLASH_XXTEA_SN_LEN);
        if( FLASH_XXTEA_SN_LEN == storage_read_sn(pdata))
        {
            memcpy(mesh_system_id,pdata,FLASH_XXTEA_ID_LEN);
            memcpy(dev_name,mesh_system_id+strlen(PRODUCT_CODE)+1,12);
            dev_name[12] = 0;
        }
        else
        {
            memset(mesh_system_id,0,FLASH_XXTEA_ID_LEN);
        }
        wiced_bt_free_buffer(pdata);
    }
    wiced_bt_cfg_settings.device_name = dev_name;

}

//*****************************************************************************
// ��������: Andon_App_Init
// ��������: switchӦ�ò��ֵĳ�ʼ��
// ��������:  None
// ��������ֵ:  None
//*****************************************************************************/
void Andon_App_Init(wiced_bool_t is_provison)
{
    extern uint32_t mesh_nvram_access(wiced_bool_t write, int inx, uint8_t* node_info, uint16_t len, wiced_result_t *p_result);
    extern void mesh_lpn_SetCallback(void (*cback)(void));
    extern void set_low_power_node_starte_idle(uint8_t idle);


    wiced_bt_device_address_t bd_addr;
    wiced_result_t            result;
    uint8_t                   provision_status;
    uint32_t                  len;

	// Johnson 2020.08.01
    // ================================================
    set_low_power_node_starte_idle(MESH_LOW_POWER_NODE_STATE_NOT_IDLE);
	// ================================================    
    
#if CHECK_BATTERY_VALUE
    wiced_hal_adc_init( );
    wiced_hal_adc_set_input_range(ADC_RANGE_0_3P6V);
    //andonBatteryIndex = 0;
    //memset(andonBatteryArray,0,ANDONBATTERYARRAYLENGTH);
#endif
    
    AndonAppSetDevInfo();
    wiced_bt_dev_read_local_addr(bd_addr);

    WICED_LOG_DEBUG("\n###########################################################\n");
    #if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
        WICED_LOG_DEBUG("Low Power Node  is_provisioned = %d, bd_addr = %B\n", is_provison, bd_addr);
        // wiced_bt_cfg_settings.device_name = dev_name;
    #elif LOW_POWER_NODE == MESH_LOW_POWER_NODE_FRIEND
        WICED_LOG_DEBUG("Friend Node  is_provisioned = %d, bd_addr = %B\n", is_provison, bd_addr);
        // wiced_bt_cfg_settings.device_name = (uint8_t *)"Andon Switch Friend";
    #else
        WICED_LOG_DEBUG("Regular Node  is_provisioned = %d, bd_addr = %B\n", is_provison, bd_addr);
        // wiced_bt_cfg_settings.device_name = dev_name;
    #endif

    //��ʼ�����������ڶ�ʱ��
    {
        WICED_LOG_DEBUG("Initial btn_state\r\n");
        mesh_btn_init();
        //example_key_init();
        WICED_LOG_DEBUG("Local name %s \n",wiced_bt_cfg_settings.device_name);
        mesh_lpn_SetCallback(Andon_App_SleepSet);
        //���������Ƿ�������������³�ʼ��
        if(wiced_is_timer_in_use(&andon_app_state.periodic_timer))  
        {
            wiced_deinit_timer(&andon_app_state.periodic_timer);
        }
        
        appconnadvenable = WICED_FALSE;
        AndonApp_runtime = 0;

        andon_app_state.run_mode = ANDON_APP_RUN_NORMAL;
        andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
        
        if( ((btn_state.btn_pairpin_level&0X03)==0X00) && ((btn_state.btn_state_level&BTN_RESET_UP_VALUE)==0) )
        {
            andon_app_state.run_mode = ANDON_APP_RUN_TEST;
            WICED_LOG_DEBUG("run_mode is ANDON_APP_RUN_TEST\r\n");
        }
        
        len = mesh_nvram_access(WICED_FALSE, NVRAM_ID_LOCAL_PROVISION, &provision_status, sizeof(uint8_t), &result);
        LOG_DEBUG("is_provisioning = %d: result = %d, len = %d\n", provision_status, result, len);
        if (result == WICED_BT_SUCCESS && len == 1 && provision_status == MESH_APP_PROVISION_STATUS_PRORESTART)
        {
            // ���ڳ�ʱδ������������лָ��������ã������ȴ�����
            mesh_nvram_access(WICED_TRUE, NVRAM_ID_LOCAL_PROVISION, NULL, 0, &result);
            andon_app_state.run_mode = ANDON_APP_RUN_WAIT_PROVISION;
            appconnadvenable = WICED_TRUE;
            appAndonBleConnectUsed();
            andon_app_state.display_mode = ANDON_APP_DISPLAY_CONFIG;
            AndonApp_pair_stata = ANDONPAIR_PAIR_WAITPROING;
            WICED_LOG_DEBUG("set_allow_sleep_flag WICED_FALSE\r\n");
            WICED_LOG_DEBUG("Local name %s \n",wiced_bt_cfg_settings.device_name);
            low_power_set_allow_sleep_flag(WICED_FALSE);
        }
        else 
        {
            WICED_LOG_DEBUG("set_allow_sleep_flag WICED_FASLE\r\n");
            low_power_set_allow_sleep_flag(WICED_FALSE);
        }
        

        AndonApp_power_stata = 0;
        wiced_init_timer(&andon_app_state.periodic_timer, &Andon_App_Periodic_Timer_Callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
        wiced_start_timer(&andon_app_state.periodic_timer, ANDON_APP_PERIODIC_TIME_LENGTH);
        andon_app_state.runing = WICED_TRUE;
        memset(&andon_app_staticstata,0,sizeof(andon_app_staticstata));
        fastturn = 0;
        fastturnflag = 0;

        keyscan_stuck_key_in_second = 8;
        //LEDY_ON;
    }
    
    //ɨ��ظ���������豸����
    {
        wiced_bt_ble_advert_elem_t  adv_elem[3];
        uint8_t                     buf[2];
        uint8_t                     num_elem = 0;
        adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
        adv_elem[num_elem].len = (uint16_t)strlen((const char*)wiced_bt_cfg_settings.device_name);
        adv_elem[num_elem].p_data = wiced_bt_cfg_settings.device_name;
        num_elem++;

        adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
        adv_elem[num_elem].len = 2;
        buf[0] = (uint8_t)wiced_bt_cfg_settings.gatt_cfg.appearance;
        buf[1] = (uint8_t)(wiced_bt_cfg_settings.gatt_cfg.appearance >> 8);
        adv_elem[num_elem].p_data = buf;
        num_elem++;
        wiced_bt_mesh_set_raw_scan_response_data(num_elem, adv_elem);
    }
    
    //��ʼ����Ʒģ��
    {
        #if USE_REMOTE_PROVISION 
            wiced_bt_mesh_remote_provisioning_server_init();
        #endif

        #if MESH_DFU
            // wiced_bt_mesh_model_fw_update_server_init(MESH_REMOTE_CLIENT_ELEMENT_INDEX, NULL, is_provison);
            // wiced_bt_mesh_model_fw_distribution_server_init();
            wiced_bt_mesh_model_fw_update_server_init("");
            wiced_bt_mesh_model_fw_distribution_server_init();
            wiced_bt_mesh_model_blob_transfer_server_init();

        #endif
            
            WICED_LOG_INF("build time: %s %s\n",__DATE__,__TIME__);
            WICED_LOG_INF("CID = 0x%04X PID = 0x%04X, VID = 0x%04X, FWID = 0x%04X\n",MESH_CID, MESH_PID, MESH_VID, MESH_FWID);
        #ifdef USE_EXTRA_API

            extern void mesh_core_node_info(void);
            extern void mesh_core_sublist_data(void);
            
            WICED_LOG_INF("Mesh Core version: %s\n", mesh_core_version_info());
            mesh_core_node_info();
            mesh_core_sublist_data();
            // mesh_core_nv_data();
        #endif
            wiced_bt_device_address_t madd;
            wiced_bt_dev_read_local_addr(madd);

            #if LOGLEVEL >= LOGLEVEL_INFO 
                WICED_BT_TRACE("local mac: %B\n",madd);
            #endif
        

        #if LOW_POWER_NODE == MESH_LOW_POWER_NODE_LPN
            if (is_provison && wiced_bt_mesh_core_lpn_get_friend_addr() != 0)
            {
                WICED_LOG_DEBUG("%s: LPN Friendship established: friend ID 0x%x\n",
                            __func__, wiced_bt_mesh_core_lpn_get_friend_addr());
            }
            else
            {
                WICED_LOG_DEBUG("%s: LPN Friendship not established!\n", __func__);
            }

            // Set low power node state change callback functions
            mesh_app_lpn_friendship_cb = mesh_remote_lpn_friendship;
        #elif LOW_POWER_NODE == MESH_LOW_POWER_NODE_FRIEND
            mesh_app_frnd_friendship_cb = mesh_remote_frnd_friendship;
        #endif
    }
    // wiced_bt_mesh_core_adv_tx_power = 3;
    // wiced_bt_dev_set_adv_tx_power(-10);
    //��δ����ʱ��ʼ���Զ���㲥����
    // if(is_provison == WICED_FALSE)
    {
        adv_pack_init();
    }

    andon_app_state.is_provison = is_provison;
    mesh_app_provision_handler = Andon_App_Provision_Stata_Change;
    WICED_LOG_DEBUG("mesh_app_interrupt_cb = 0x%X\n", mesh_app_interrupt_cb);
    
    WICED_LOG_DEBUG("Free memory size = %d\n", wiced_memory_get_free_bytes());
    #if ANDON_TEST
    void storage_save_keyandsn(void);
    void PrintfXxteakeyandsn(void);
    PrintfXxteakeyandsn();
    storage_save_keyandsn();
    #endif

    AndonApppaired = WICED_FALSE;
    if(sizeof(AndonApppaired) != flash_app_read(FLASH_ADDR_STATE, (uint8_t*)(&AndonApppaired), sizeof(AndonApppaired)))
    {
        AndonApppaired = WICED_FALSE;
    }
    if(AndonApppaired != WICED_TRUE){
        AndonApppaired = WICED_FALSE;
    }
    WICED_LOG_DEBUG("###########################################################\n");
}

void andon_app_encoder_action(void)
{
    if(AndonApp_runtime > 4)
    {
        andon_app_staticstata.btn_encoder_numcnt ++;

        if(andon_app_staticstata.btn_encoder_start_time == 0){
            // andon_app_staticstata.btn_encoder_start_time = AndonApp_runtime;
            andon_app_staticstata.btn_encoder_start_time = clock_SystemTimeMicroseconds64();
            LOG_DEBUG("Start %d us\n",andon_app_staticstata.btn_encoder_start_time);
        }else{
            // andon_app_staticstata.btn_encoder_end_time = AndonApp_runtime;
            andon_app_staticstata.btn_encoder_end_time = clock_SystemTimeMicroseconds64();
            LOG_DEBUG("end %d us\n",andon_app_staticstata.btn_encoder_end_time);
        }
    }
    
}

//*****************************************************************************
// ��������: 
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
void Andon_App_Run_Normal_Mode(void)
{
    static int16_t first_step = 0;
    static int16_t pressturn = 0;
    static int16_t singleclick = 0;
    uint16_t doubleclicktime = 0;

    andon_app_staticstata.btn_onoff_lasttime++;
    //�����������1s����Ϊ����������
    // if(andon_app_staticstata.btn_onoff_lasttime > 1000/ANDON_APP_PERIODIC_TIME_LENGTH){
    //     if(andon_app_staticstata.btn_fastpress_numcnt == 1){
    //         AndonCmd_Action(enumREMOTEACTION_SHORTPRESS,0,ANDONCMD_TRANS_TIME);
    //     }
    //     andon_app_staticstata.btn_fastpress_numcnt = 0;
    //     andon_app_staticstata.btn_onoff_lasttime = 1000/ANDON_APP_PERIODIC_TIME_LENGTH;
    //     andon_app_staticstata.send_onoff_timecnt = 0;
    // }
    if(AndonApp_runtime > 300/ANDON_APP_PERIODIC_TIME_LENGTH){
        doubleclicktime = 300/ANDON_APP_PERIODIC_TIME_LENGTH;
    }else{
        doubleclicktime = 200/ANDON_APP_PERIODIC_TIME_LENGTH;
    }

    doubleclicktime = 0;
    // if(andon_app_staticstata.btn_onoff_lasttime > doubleclicktime)
    {
        if( (andon_app_staticstata.btn_onoff_lasttime == (doubleclicktime+1))
            && (andon_app_staticstata.btn_fastpress_numcnt == 1)
            && (singleclick == 0) && (pressturn == 0)){
            // AndonCmd_Toggle(0);
            AndonCmd_Action(enumREMOTEACTION_SHORTPRESS,0,ANDONCMD_TRANS_TIME);
            singleclick = 1;
        }
        if(andon_app_staticstata.btn_onoff_lasttime > 1000/ANDON_APP_PERIODIC_TIME_LENGTH){
            andon_app_staticstata.btn_fastpress_numcnt = 0;
            andon_app_staticstata.btn_onoff_lasttime = 1000/ANDON_APP_PERIODIC_TIME_LENGTH;
            andon_app_staticstata.send_onoff_timecnt = 0;
            singleclick = 0;
        }
    }

    if(andon_app_staticstata.btn_fastpress_numcnt > 0){
        //������������1s�ļ������ONOFFָ��
        andon_app_staticstata.send_onoff_timecnt++;
        // if(andon_app_staticstata.send_onoff_timecnt > 2500/ANDON_APP_PERIODIC_TIME_LENGTH)
        // {
        //     andon_app_staticstata.send_onoff_timecnt = 1;
        //     //TODO ����ONOFFָ��
        //     if(andon_app_staticstata.btn_onoff_cmdstata)
        //     {
        //         andon_app_staticstata.btn_onoff_cmdstata = WICED_FALSE;
        //     } 
        //     else
        //     {
        //         andon_app_staticstata.btn_onoff_cmdstata = WICED_TRUE;
        //     }
        //     AndonCmd_Onoff(andon_app_staticstata.btn_onoff_cmdstata,0);
        // }
    }
    if((btn_state.btn_press_count[BTN_ONOFF_INDEX] == 5000/ANDON_APP_PERIODIC_TIME_LENGTH) && (0 == pressturn)){
        //AndonCmd_Onoff(0,0xFFFF);
        AndonCmd_Action(enumREMOTEACTION_LONGPRESS,0,ANDONCMD_TRANS_TIME);
    } 
    if(btn_state.btn_idle_count > 300/ANDON_APP_PERIODIC_TIME_LENGTH){
        andon_app_staticstata.btn_encoder_start_time = 0;
        andon_app_staticstata.btn_encoder_end_time   = 0;
        andon_app_staticstata.btn_encoder_numcnt     = 0;
        fastturn = 0;
        fastturnflag = 0;
        if(btn_state.btn_press_count[BTN_ONOFF_INDEX] == 0) {
            pressturn = 0;
        }
    }
    if(btn_state.btn_up & BTN_ONOFF_VALUE){
        LOG_DEBUG("1111111111111111111111.......................\n");
        if(pressturn == 0){
        //     pressturn = 0;
        // }else
        // {
            andon_app_staticstata.btn_onoff_lasttime = 0;
            if(btn_state.btn_press_count[BTN_ONOFF_INDEX] < 1000/ANDON_APP_PERIODIC_TIME_LENGTH){
                //andon_app_staticstata.btn_onoff_lasttime = 0;
                andon_app_staticstata.btn_fastpress_numcnt++;
                if((andon_app_staticstata.btn_fastpress_numcnt < 4) && (singleclick==0)){
                    // AndonCmd_Toggle(0);
                    // AndonCmd_Action(enumREMOTEACTION_SHORTPRESS,0,ANDONCMD_TRANS_TIME);
                    if(andon_app_staticstata.btn_fastpress_numcnt == 2){
                        AndonCmd_Action(enumREMOTEACTION_DOUBLECLICK,0,ANDONCMD_TRANS_TIME);
                    // }else if(andon_app_staticstata.btn_fastpress_numcnt == 3){
                    //     AndonCmd_Action(enumREMOTEACTION_THREECLICK,0,ANDONCMD_TRANS_TIME);
                    }
                // }else if(andon_app_staticstata.btn_fastpress_numcnt < 5){
                //     if(andon_app_staticstata.btn_fastpress_numcnt == 2){
                //         AndonCmd_Action(enumREMOTEACTION_SHORTPRESS,0,ANDONCMD_TRANS_TIME);
                //     }
                //     else if(andon_app_staticstata.btn_fastpress_numcnt == 4){
                //         andon_app_staticstata.btn_onoff_cmdstata = WICED_FALSE;
                //         AndonCmd_Onoff(andon_app_staticstata.btn_onoff_cmdstata,0);
                //     }
                }else{
                    if(andon_app_staticstata.send_onoff_timecnt > 450/ANDON_APP_PERIODIC_TIME_LENGTH){
                        andon_app_staticstata.send_onoff_timecnt = 1;
                        // if(andon_app_staticstata.btn_onoff_cmdstata){
                        //     andon_app_staticstata.btn_onoff_cmdstata = WICED_FALSE;
                        // }else{
                        //     andon_app_staticstata.btn_onoff_cmdstata = WICED_TRUE;
                        // }
                        // AndonCmd_Onoff(andon_app_staticstata.btn_onoff_cmdstata,0);
                        AndonCmd_Action(enumREMOTEACTION_SHORTPRESS,0,ANDONCMD_TRANS_TIME);
                    }
                    // else if(andon_app_staticstata.send_onoff_timecnt > 1500/ANDON_APP_PERIODIC_TIME_LENGTH)
                    // {
                    //     andon_app_staticstata.send_onoff_timecnt = 1;
                    //     //TODO ����ONOFFָ��
                    //     if(andon_app_staticstata.btn_onoff_cmdstata)
                    //     {
                    //         andon_app_staticstata.btn_onoff_cmdstata = WICED_FALSE;
                    //     } 
                    //     else
                    //     {
                    //         andon_app_staticstata.btn_onoff_cmdstata = WICED_TRUE;
                    //     }
                    //     AndonCmd_Onoff(andon_app_staticstata.btn_onoff_cmdstata,0);
                    // }
                }
            }
        }
    }
    #if !ENCODER
        if(btn_state.btn_up & BTN_LIGHTNESS_DOWN_VALUE)
        {
            if(btn_state.btn_press_count[BTN_LIGHTNESS_DOWN_INDEX] > 40/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                //TODO �������ȵ���ָ��
            }
        }
        if(btn_state.btn_up & BTN_LIGHTNESS_UP_VALUE)
        {
            if(btn_state.btn_press_count[BTN_LIGHTNESS_UP_INDEX] > 40/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                //TODO �������ȵ���ָ��
            }
        }
    #else
        if(btn_state.btn_encoder_count)
        {
            //btn_state.btn_idle_count = 0;  //��������ʱ�����ð����ͷ�ʱ��
            
            if(AndonApp_power_stata < 3)
            {
                AndonApp_power_stata = 4;
                andon_app_staticstata.send_delta_timecnt = (ANDONAPP_DELTACMD_INTERVAL-100)/ANDON_APP_PERIODIC_TIME_LENGTH;
            }
            andon_app_staticstata.send_delta_timecnt++;
            if(andon_app_staticstata.send_delta_timecnt > ANDONAPP_DELTACMD_INTERVAL/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                int16_t delta_plus;
                
                // if(btn_state.btn_ghost == WICED_TRUE){
                //     btn_state.btn_ghost = WICED_FALSE;
                //     fastturn = 1;
                // }

                //���㵥�������ʱ�䣬���С���趨ֵ����Ϊ�ǿ�����ת
                if(andon_app_staticstata.btn_encoder_end_time){
                    if(andon_app_staticstata.btn_encoder_end_time > andon_app_staticstata.btn_encoder_start_time){
                        uint32_t deltatime;
                        deltatime = andon_app_staticstata.btn_encoder_end_time - andon_app_staticstata.btn_encoder_start_time;
                        //����btn_encoder_numcnt��Ϊ�ӣ����Բ������0�����
                        if(deltatime/andon_app_staticstata.btn_encoder_numcnt < 20000){
                            fastturn = 1;
                        }
                    }
                }
                // int16_t delta_plus;
                // uint16_t delta_time;
                // uint16_t delta_rate;

                // if((btn_state.btn_encoder_count < 2) && (btn_state.btn_encoder_count > -2))
                // {
                //     delta_plus = btn_state.btn_encoder_count;
                // }
                // else 
                // {
                //     delta_time = (uint16_t) ((andon_app_staticstata.btn_encoder_end_time - andon_app_staticstata.btn_encoder_start_time)/1000);
                //     if(btn_state.btn_encoder_count < 0)
                //         delta_rate = -btn_state.btn_encoder_count - 1;
                //     else
                //     {
                //         delta_rate = btn_state.btn_encoder_count - 1;
                //     }
                //     delta_rate = delta_rate*1000/delta_time;
                //     if(delta_rate < 10)
                //     {
                //         delta_plus = btn_state.btn_encoder_count * 4;
                //     }
                //     else if(delta_rate < 20)
                //     {
                //         delta_plus = btn_state.btn_encoder_count * 7;
                //     }
                //     else 
                //     {
                //         delta_plus = btn_state.btn_encoder_count * 15;
                //     }
                // }
                // andon_app_staticstata.btn_encoder_start_time = 0;
                // andon_app_staticstata.btn_encoder_end_time   = 0;
                // if(delta_plus != 0)
                // {
                //     AndonCmd_Delta(delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                // }
                // andon_app_staticstata.send_delta_timecnt = 0;
                // btn_state.btn_encoder_count= 0;


                // if((btn_state.btn_encoder_count < 2) && (btn_state.btn_encoder_count > -2))
                // {
                //     delta_plus = btn_state.btn_encoder_count;
                // }
                // else if((btn_state.btn_encoder_count < 3) && (btn_state.btn_encoder_count > -3))
                // {
                //     delta_plus = btn_state.btn_encoder_count*5;
                // }
                // else 
                if(btn_state.btn_encoder_count < 0){
                    delta_plus = -btn_state.btn_encoder_count;
                }else{
                    delta_plus = btn_state.btn_encoder_count;
                }
                
                if( (AndonApp_power_stata == 4) && (delta_plus < 7))
                {
                    WICED_LOG_DEBUG("btn_state.btn_encoder_count = %d\n", btn_state.btn_encoder_count);
                    andon_app_staticstata.send_delta_timecnt = 100/ANDON_APP_PERIODIC_TIME_LENGTH;
                    AndonApp_power_stata = 5;
                    first_step = btn_state.btn_encoder_count;
                    //zhw 20200630 ֱ�Ӵ���deltaֵ start
                    delta_plus = btn_state.btn_encoder_count; 
                    // delta_plus = btn_state.btn_encoder_count*10;  
                    //zhw 20200630 ֱ�Ӵ���deltaֵ end

                    if(delta_plus != 0) 
                    {
                        if(fastturn){
                            if(delta_plus < 0){
                                if(fastturnflag == 2){
                                    goto APP_NORMOL_ENCODE_DONE;
                                }
                                delta_plus = -50;
                                fastturnflag = 2;
                            }else{
                                if(fastturnflag == 1){
                                    goto APP_NORMOL_ENCODE_DONE;
                                }
                                delta_plus = 50;
                                fastturnflag = 1;
                            }
                        }
                        if((btn_state.btn_press_count[BTN_ONOFF_INDEX]) || (pressturn)){
                            pressturn = 1;
                            AndonCmd_Action(enumREMOTEACTION_PRESSTURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                        }else{
                            AndonCmd_Action(enumREMOTEACTION_TURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                        }
                        //AndonCmd_Delta(delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                    }
                }
                else
                {
                    WICED_LOG_DEBUG("btn_state.btn_encoder_count = %d\n", btn_state.btn_encoder_count);
                    
                    //zhw 20200630 ֱ�Ӵ���deltaֵ Start
                    // if((delta_plus < 7) || ((andon_app_staticstata.btn_encoder_end_time - andon_app_staticstata.btn_encoder_start_time) > 600000))
                    // {
                    //     if(first_step != 0)
                    //     {
                    //         btn_state.btn_encoder_count -= first_step;
                    //         first_step = 0;
                    //     }
                    //     delta_plus = btn_state.btn_encoder_count*10;
                    // }
                    // else 
                    // {
                    //     delta_plus = btn_state.btn_encoder_count*10;
                    // }
                    
                    if(first_step != 0)
                    {
                        btn_state.btn_encoder_count -= first_step;
                        first_step = 0;
                    }
                    delta_plus = btn_state.btn_encoder_count;
                    //zhw 20200630 ֱ�Ӵ���deltaֵ end
                    if(delta_plus != 0) 
                    {
                        if(fastturn){
                            if(delta_plus < 0){
                                if(fastturnflag == 2){
                                    goto APP_NORMOL_ENCODE_DONE;
                                }
                                delta_plus = -50;
                                fastturnflag = 2;
                            }else{
                                if(fastturnflag == 1){
                                    goto APP_NORMOL_ENCODE_DONE;
                                }
                                delta_plus = 50;
                                fastturnflag = 1;
                            }
                        }
                        if(AndonApp_power_stata == 4)
                        {
                            AndonApp_power_stata = 5;
                            // AndonCmd_Delta(delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            if((btn_state.btn_press_count[BTN_ONOFF_INDEX]) || (pressturn)){
                                pressturn = 1;
                                AndonCmd_Action(enumREMOTEACTION_PRESSTURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            }else{
                                AndonCmd_Action(enumREMOTEACTION_TURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            }
                        }
                        else
                        {
                            // AndonCmd_Delta(delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            if((btn_state.btn_press_count[BTN_ONOFF_INDEX]) || (pressturn)){
                                pressturn = 1;
                                AndonCmd_Action(enumREMOTEACTION_PRESSTURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            }else{
                                AndonCmd_Action(enumREMOTEACTION_TURN,delta_plus,ANDONAPP_DELTACMD_INTERVAL*5/4);
                            }
                        }
                    }
APP_NORMOL_ENCODE_DONE:
                    first_step = 0;
                    // andon_app_staticstata.btn_encoder_start_time = 0;
                    // andon_app_staticstata.btn_encoder_end_time = 0;
                    andon_app_staticstata.send_delta_timecnt = 0;
                    btn_state.btn_encoder_count= 0;
                }
            }
        }
    #endif
}


extern wiced_platform_button_config_t platform_button[];
extern void wiced_hal_wdog_reset_system(void);
extern wiced_bool_t clear_flash_for_reset(wiced_bt_mesh_core_config_t *p_config_data,wiced_bt_core_nvram_access_t nvram_access_callback);
extern void wdog_generate_hw_reset(void);
//*****************************************************************************
// ��������: Andon_App_Periodic_Timer_Callback
// ��������: 
// ��������:  
// ��������ֵ: 
//*****************************************************************************/
static void Andon_App_Periodic_Timer_Callback(uint32_t arg)
{
    static uint16_t reset_flag = 0;
    static uint16_t fact_reset_delay = 0;
    static uint16_t displaytime = 0;
    
#if CHECK_BATTERY_VALUE
    static uint16_t lowBatterycnt = 0;
    static uint16_t lowBatteryFlag = 0;

    if(wiced_hal_adc_read_voltage( CHANNEL_TO_MEASURE_DC_VOLT) < 1000)
    {
        lowBatterycnt++;
    }
    else if(lowBatteryFlag == 1)
    {
        lowBatterycnt--;
        if(lowBatterycnt == 0)
        {
            lowBatterycnt = 0;
            lowBatteryFlag = 0;
            if(ANDON_APP_RUN_NORMAL == andon_app_state.run_mode )
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            }
        }
    }
    if(lowBatterycnt > 5)
    {
        lowBatterycnt = 5;
        lowBatteryFlag = 1;
    }
    //WICED_LOG_DEBUG("ADC Value %d\n",wiced_hal_adc_read_voltage( CHANNEL_TO_MEASURE_DC_VOLT));
#endif


    if(andon_app_state.runing == WICED_FALSE)  
    {
        reset_flag++;
        if(reset_flag > 20100/ANDON_APP_PERIODIC_TIME_LENGTH)
        { 
            wiced_hal_wdog_reset_system();
            return;
        }
        //�������20s��ϵͳ��û����������ΪЭ��ջ���Զ���SDS����ϵͳ����
        else if(reset_flag > 20000/ANDON_APP_PERIODIC_TIME_LENGTH)
        {
            //���³�ʼ���������е�timer��ʹϵͳ���������ַ�ʽ��reset��
            wiced_init_timer(&andon_app_state.periodic_timer, &Andon_App_Periodic_Timer_Callback, 0, WICED_MILLI_SECONDS_PERIODIC_TIMER);
            return;
        }
    }
    else 
    {
        reset_flag = 0;
    }
    
    if(fact_reset_delay)
    {
        fact_reset_delay ++;
        if(fact_reset_delay > 1500/ANDON_APP_PERIODIC_TIME_LENGTH)
        {
            uint32_t mesh_nvram_access(wiced_bool_t write, int inx, uint8_t* node_info, uint16_t len, wiced_result_t *p_result);
            //��дң�������ָ��������õ�index
            advpackReWriteRemoteIndex();
            StoreBindKey(NULL,0);
            AndonApppaired = WICED_FALSE;
            flash_write_erase(FLASH_ADDR_STATE, (uint8_t*)(&AndonApppaired), sizeof(AndonApppaired),WICED_TRUE);
            // mesh_application_factory_reset();
            clear_flash_for_reset(&mesh_config,mesh_nvram_access);
            wdog_generate_hw_reset();
            while(1);
        }
    }

    if((andon_app_state.display_mode == ANDON_APP_DISPLAY_TEST_UPGRADEOK)
        && (andon_app_state.run_mode == ANDON_APP_RUN_TEST)){  //��װ����������ɣ�������ʾ
#if CHECK_BATTERY_VALUE
        if(lowBatteryFlag == 1) {
            andon_app_state.display_mode = ANDON_APP_DISPLAY_LOWPOWER;
        }
#endif
        DisplayRefresh((AndonApppaired==WICED_TRUE)?WICED_TRUE:andon_app_state.is_provison);
        return;
    }

    wiced_btn_scan();
    AndonApp_runtime ++;
    if(AndonApp_runtime > 0x08000000){
        AndonApp_runtime = 0x08000000;
    }
    if(0 == AndonApp_power_stata)
    {
        AndonApp_power_stata = 1;
        if(ANDON_APP_RUN_TEST == andon_app_state.run_mode )
        {
            if( ((btn_state.btn_pairpin_level&0X03)==0X00) && ((btn_state.btn_state_level&BTN_RESET_UP_VALUE)==0) )
            {
                #include "tooling.h"
                andon_app_state.run_mode = ANDON_APP_RUN_TEST;
                andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_DOING;
                tooling_init();
                low_power_set_allow_sleep_flag(WICED_FALSE);
            }
            else 
            {
                andon_app_state.run_mode = ANDON_APP_RUN_NORMAL;
            }
        }
    }
    
    if(ANDON_APP_RUN_TEST == andon_app_state.run_mode)
    {
        #if CHECK_BATTERY_VALUE    
            if(lowBatteryFlag == 1) 
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_LOWPOWER;
            }
        #endif
        if(btn_state.btn_press_count[BTN_ONOFF_INDEX])
        {
            if(btn_state.btn_press_count[BTN_ONOFF_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH){
                andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET3S;
            }
            else{
                andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_PRESS_CONFIG;
            }
        }
        if( btn_state.btn_up & BTN_ONOFF_VALUE) 
        {
            if(btn_state.btn_press_count[BTN_ONOFF_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                AndonPair_ResetFactor1();
                andon_app_state.display_mode = ANDON_APP_DISPLAY_TEST_DOING;
            }
        }
        
        DisplayRefresh(andon_app_state.is_provison);
        if((btn_state.btn_pairpin_level&0X0F) == 0X0F)
        {
            andon_app_state.run_mode = ANDON_APP_RUN_NORMAL;
            btn_state.btn_idle_count = 2500/ANDON_APP_PERIODIC_TIME_LENGTH;
            low_power_set_allow_sleep_flag(WICED_TRUE);
        }
        return;
    }

    if( (btn_state.btn_pairpin_level&0X0F) == 0X00)    //������������
    {
        #ifdef ANDON_BEEP
        if(AndonAppBeepTimer == 0)
        {
            AnondBeepOn(3,200,100);
        }
        AndonAppBeepTimer++;
        if(AndonAppBeepTimer > 10000/ANDON_APP_PERIODIC_TIME_LENGTH)
        {
            AndonAppBeepTimer = 0;
        }
        #endif // DEBUG
        if(ANDON_APP_RUN_NORMAL == andon_app_state.run_mode)  //��������ǰ���ڳ���ģʽ
        {
            keyscan_stuck_key_in_second = 240;
            WICED_LOG_DEBUG("set_allow_sleep_flag WICED_FALSE\r\n");
            low_power_set_allow_sleep_flag(WICED_FALSE);
            if(WICED_TRUE == andon_app_state.is_provison)
            {
                andon_app_state.run_mode = ANDON_APP_RUN_APP_PROVISION;
                //����ң������Թ���
                //AndonPair_DeInit(WICED_FALSE);
                AndonPair_Init(Andon_App_AdvPairDone);
            }
            else
            {
                andon_app_state.run_mode = ANDON_APP_RUN_WAIT_PROVISION;
                appconnadvenable = WICED_TRUE;
                appAndonBleConnectUsed();
                LOG_DEBUG("start wyze Adv!!!!! \n");
                //�����Թ��ܳ�ʼ��
                AndonPair_Init(Andon_App_AdvPairDone);
            }
            WICED_LOG_DEBUG("Goto ANDON_APP_DISPLAY_PAIR_DOING\n");
            // AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
            AndonApp_pair_stata = ANDONPAIR_PAIR_DOING;
            displaytime = 0;
        }
        
    }
    else if( ((btn_state.btn_pairpin_level&0X0F) == 0X0F) 
             && (ANDON_APP_RUN_NORMAL != andon_app_state.run_mode) )
    {
        AndonPair_Stop();
        andon_app_state.run_mode = ANDON_APP_RUN_NORMAL;
        appconnadvenable = WICED_FALSE;
        andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;

        WICED_LOG_DEBUG("1.....!!!\n");
        //����ң������Թ���
        AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
        //����Appʱֱ�ӶϿ�����
        if(mesh_app_gatt_is_connected()){
            mesh_app_gatt_is_disconnected();
        }
        AndonPair_DeInit(WICED_TRUE);
        btn_state.btn_idle_count = 0;
        btn_state.btn_encoder_count = 0;
    }
    
    // if( (ANDON_APP_DISPLAY_NORMAL == andon_app_state.display_mode) 
    //     || (ANDON_APP_DISPLAY_CONFIG == andon_app_state.display_mode) 
    //     ||  (ANDON_APP_DISPLAY_BTN_PRESS == andon_app_state.display_mode) )
    // {
    //     // if( (BTN_RELEASE_MASK != (btn_state.btn_state_level&BTN_RELEASE_MASK))
    //     //     || (0x00 != btn_state.btn_up)
    //     //     || (0 != btn_state.btn_encoder_count) )
    //     if(btn_state.btn_idle_count < 200/ANDON_APP_PERIODIC_TIME_LENGTH)
    //     {
    //         andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_PRESS;
    //     }
    //     else 
    //     {
    //         andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
    //     }
    // }
    
    //����̬����״̬����
    if(ANDON_APP_RUN_NORMAL != andon_app_state.run_mode)
    {
        //btn_state.btn_idle_count = 0;

        if( (ANDON_APP_DISPLAY_CONFIG == andon_app_state.display_mode) 
            || (ANDON_APP_DISPLAY_BTN_PRESS_CONFIG == andon_app_state.display_mode) )
        {
            if ( (btn_state.btn_pressed == WICED_TRUE) 
                 && (btn_state.btn_idle_count < 200/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_PRESS_CONFIG;
            }
            else 
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_CONFIG;
            }
        }
        // if(btn_state.btn_press_count[BTN_RESET_INDEX] > 10000/ANDON_APP_PERIODIC_TIME_LENGTH) 
        // {
        //     andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET10S;
        // }
        // else if(btn_state.btn_press_count[BTN_RESET_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
        // {
        //     andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET3S;
        // }
        // else if(ANDON_APP_DISPLAY_NORMAL == andon_app_state.display_mode)
        // {
        //     andon_app_state.display_mode = ANDON_APP_DISPLAY_CONFIG;
        // }

        for(uint16_t i= 0; i<BTN_NUM; i++)
        {
            if(btn_state.btn_press_count[i] > 10000/ANDON_APP_PERIODIC_TIME_LENGTH) 
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET10S;
            }
            else if(btn_state.btn_press_count[i] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
            {
                if(andon_app_state.display_mode != ANDON_APP_DISPLAY_BTN_RESET10S)
                {
                    andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET3S;
                }
            }
            else if(ANDON_APP_DISPLAY_NORMAL == andon_app_state.display_mode)
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_CONFIG;
            }
        }
    }
    else
    {
        if( (ANDON_APP_DISPLAY_NORMAL == andon_app_state.display_mode) 
            || (ANDON_APP_DISPLAY_BTN_PRESS_NORMAL == andon_app_state.display_mode) )
        {
            if ( (btn_state.btn_pressed == WICED_TRUE) 
                 && (btn_state.btn_idle_count < 100/ANDON_APP_PERIODIC_TIME_LENGTH) ) 
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_PRESS_NORMAL;
            }
            else 
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            }
        }
        if(btn_state.btn_press_count[BTN_RESET_INDEX] > 10000/ANDON_APP_PERIODIC_TIME_LENGTH) 
        {
            andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET10S;
        }
        else if(btn_state.btn_press_count[BTN_RESET_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
        {
            andon_app_state.display_mode = ANDON_APP_DISPLAY_BTN_RESET3S;
        }
    }

#if CHECK_BATTERY_VALUE    
    if((lowBatteryFlag == 1) && (ANDON_APP_RUN_NORMAL == andon_app_state.run_mode)) 
    {
        andon_app_state.display_mode = ANDON_APP_DISPLAY_LOWPOWER;
    }
#endif
    // if(AndonApppaired == WICED_TRUE){
    //     andon_app_state.is_provison = WICED_TRUE;
    // }
    DisplayRefresh((AndonApppaired==WICED_TRUE)?WICED_TRUE:andon_app_state.is_provison);
    //App����&&1s�ް���&&��ͨ����ģʽ
    if( ANDON_APP_RUN_NORMAL == andon_app_state.run_mode) 
    {
        if ((btn_state.btn_idle_count > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
            && ((btn_state.btn_pairpin_level &0X0F) == 0X0F) )
        {
            //�͵�ģʽ����������
            if((WICED_TRUE == andon_app_state.runing) && (andon_app_state.display_mode != ANDON_APP_DISPLAY_LOWPOWER))
            {
                if(!mesh_app_node_is_provisioned())
                {
                    adv_manuDevAdvStop();
                }
                WICED_LOG_DEBUG("set_allow_sleep_flag WICED_TRUE\r\n");
                low_power_set_allow_sleep_flag(WICED_TRUE);
                andon_app_state.runing = WICED_FALSE;
                appconnadvenable = WICED_FALSE;
                WICED_LOG_DEBUG("Goto sleep\n");
                wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF,BLE_ADDR_RANDOM ,NULL);
            }
        }
        else
        {
            if(AndonApp_power_stata < 0xFFFE)
            {
                andon_app_state.runing = WICED_TRUE;
            }
        }
        
    }
    else
    {
        andon_app_state.runing = WICED_TRUE;
    }
    
    
    switch(andon_app_state.run_mode)
    {
        case ANDON_APP_RUN_NORMAL:          //��ͨģʽ ʶ�𵽰���֮����ָ�� 
        {
            //�ް���&&�ǵ͵�
            if(btn_state.btn_idle_count < 2)  
            {
                WICED_LOG_DEBUG("set_allow_sleep_flag WICED_FALSE\r\n");
                low_power_set_allow_sleep_flag(WICED_FALSE);
            }
            //WICED_LOG_DEBUG("set_allow_sleep_flag WICED_TRUE\r\n");
            //low_power_set_allow_sleep_flag(WICED_TRUE);
            Andon_App_Run_Normal_Mode();
            if( btn_state.btn_up & BTN_RESET_UP_VALUE) 
            {
                if(btn_state.btn_press_count[BTN_RESET_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    AndonPair_Unband();
                    fact_reset_delay = 1;
                    andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                    AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
                }
            }
            break;
        }
        case ANDON_APP_RUN_WAIT_PROVISION:  //��ģʽ�µȴ��������ڵȴ����������а���ON/OFF��������ң�������
        case ANDON_APP_RUN_APP_PROVISION:
        {
            btnKeyScanStart();
            //δ����ʱ����ON/OFF���������
            if(btn_state.btn_up & BTN_ONOFF_VALUE) 
            {
                WICED_LOG_DEBUG("ON/OFF btn release  pressed time %d!!!\n",btn_state.btn_press_count[BTN_ONOFF_INDEX]);
                if(btn_state.btn_press_count[BTN_ONOFF_INDEX] > 10000/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    //ֹͣ���
                    AndonPair_Stop();
                    AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
                    andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                }
                else if(btn_state.btn_press_count[BTN_ONOFF_INDEX] > 40/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    // if(AndonApp_pair_stata == ANDONPAIR_PAIR_IDLE)
                    // {
                    //     //�����������
                    //     AndonPair_Start();
                    //     WICED_LOG_DEBUG("Goto ANDON_APP_DISPLAY_PAIR_DOING\n");
                    //     AndonApp_pair_stata = ANDONPAIR_PAIR_DOING;
                    //     displaytime = 0;
                    // }
                    // else 
                    if(AndonApp_pair_stata == ANDONPAIR_PAIR_DOING)
                    {
                        //ȷ�����
                        AndonPair_Paired(); 
                        AndonApp_pair_stata = ANDONPAIR_PAIR_DONE;
                        displaytime = 0;
                    }
                }
            }
                        
            if((btn_state.btn_encoder_count) && (btn_state.btn_idle_count > 500/ANDON_APP_PERIODIC_TIME_LENGTH))
            {
                if(AndonApp_pair_stata == ANDONPAIR_PAIR_IDLE)  //����״̬ ��������
                {
                    // AndonPair_Start();
                    AndonPair_Init(Andon_App_AdvPairDone);
                    WICED_LOG_DEBUG("Goto ANDON_APP_DISPLAY_PAIR_DOING\n");
                    AndonApp_pair_stata = ANDONPAIR_PAIR_DOING;
                    displaytime = 0;
                }
                else if(AndonApp_pair_stata == ANDONPAIR_PAIR_DOING)  //���ڷ����ҵ�ָ��Ĺ����У���ת�л���
                {
                    WICED_LOG_DEBUG("next\n");
                    AndonPair_Next(btn_state.btn_encoder_count);
                    btn_state.btn_encoder_count = 0;
                }
            }
            
            // ����ڰ�����Թ�����
            if(ANDONPAIR_PAIR_DOING == AndonApp_pair_stata)
            {
                andon_app_state.display_mode = ANDON_APP_DISPLAY_PAIR_DOING;
            }
            // ����������
            else if(ANDONPAIR_PAIR_DONE == AndonApp_pair_stata)
            {
                displaytime++;
                if(displaytime > 1800/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    displaytime = 0;
                    AndonApp_pair_stata = ANDONPAIR_PAIR_DOING;
                    //AndonPair_Next(1);
                    WICED_LOG_DEBUG("Goto ANDON_APP_DISPLAY_DOING\n");
                }
                // andon_app_state.display_mode = ANDON_APP_DISPLAY_PAIR_SUCCESS;

            }
            // ���ʧ��
            else if(ANDONPAIR_PAIR_FAILED == AndonApp_pair_stata)
            {
                displaytime++;
                if(displaytime > 1800/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    displaytime = 0;
                    AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
                    WICED_LOG_DEBUG("Goto ANDON_APP_DISPLAY_NORMAL\n");
					andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
					btn_state.btn_encoder_count = 0;
                }
                // andon_app_state.display_mode = ANDON_APP_DISPLAY_PAIR_FAILED;
            }
            else
            {
                if(btn_state.btn_idle_count  > 500/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    btn_state.btn_encoder_count = 0;
                }
                //andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
            }
            if( btn_state.btn_up & BTN_RESET_UP_VALUE) 
            {
                if(btn_state.btn_press_count[BTN_RESET_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
                {
                    AndonPair_Unband();
                    fact_reset_delay = 1;
                    andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                    AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
                }
                // else if(btn_state.btn_press_count[BTN_RESET_INDEX] > 3000/ANDON_APP_PERIODIC_TIME_LENGTH)
                // {
                //     mesh_application_factory_reset();
                //     //AndonCmd_Reset();
                //     andon_app_state.display_mode = ANDON_APP_DISPLAY_NORMAL;
                //     AndonApp_pair_stata = ANDONPAIR_PAIR_IDLE;
                // }
            }
            
            break;
        }
        case ANDON_APP_RUN_BAKGROUD_PROVISION:
        {
            break;
        }
    }
}


static const char     *DEFAULT_GROUP_ALL_NAME = "ALL";
static const uint16_t  DEFAULT_GROUP_ALL_ADDR = 0xC000;
const char *wiced_bt_mesh_vendor_get_default_group_name(void)
{
    WICED_LOG_DEBUG("Vendor Get Default Group Name!!!\n");
    return DEFAULT_GROUP_ALL_NAME;
}

uint16_t wiced_bt_mesh_vendor_get_default_group_addr(void)
{
    WICED_LOG_DEBUG("Vendor Get Default Group addr!!!\n");
    return DEFAULT_GROUP_ALL_ADDR;
}


void appSetConnAdvEnable(void)
{
    appconnadvenable = WICED_TRUE;
}

void appSetConnAdvDisable(void)
{
    if(appconnadvenable == WICED_TRUE)
    {
        AndonPair_Stop();
        AndonPair_DeInit(WICED_FALSE);
    }
    low_power_set_allow_sleep_flag(WICED_TRUE);
    wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF, BLE_ADDR_RANDOM, NULL );
    // wiced_bt_mesh_core_provisioning_srv_adv_interval = 3200;
    appconnadvenable = WICED_FALSE;
}

wiced_bool_t appAndonBleConnectUsed(void)
{
    //if((andon_app_state.run_mode != ANDON_APP_RUN_WAIT_PROVISION) || (appconnadvenable == WICED_FALSE))
    if(appconnadvenable == WICED_FALSE)
    {
        LOG_DEBUG("2.....  run_mode  %d  appconnadvenable %d\n",andon_app_state.run_mode,appconnadvenable);
        return WICED_FALSE;
    }
    {
        wiced_bt_ble_advert_elem_t adv_elem[2];
        uint8_t *dev_name;
        uint8_t num_elem = 0;
        uint8_t buf[2];
        uint8_t flag = BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED;
        
        {
            static uint8_t devdata[16] = {0x70,0x08,0x03,0x07};
            // static uint8_t devdata[16] = {0x70,0x08,0x01,0x02};
            uint16_t node_id = 0;

            // wiced_bt_dev_read_local_addr(devdata+4);

            //�˴�����mac����DID�е�mac����һ��
            LOG_DEBUG("mesh_system_id: %s\n",mesh_system_id);
            for(uint8_t i=0;i<12;i+=2)
            {
                devdata[i/2+4] =  HexStr2Int8U(mesh_system_id+i+strlen(PRODUCT_CODE)+1);
            }

            // if(mesh_app_node_is_provisioned())
            if(storageBindkey.bindflag == WICED_TRUE)
            {
                devdata[10] = 0x01;
            }
            else
            {
                devdata[10] = 0x00;
            }
            devdata[11] = 0x01;
            // node_id = wiced_bt_mesh_core_get_local_addr();
            devdata[12] = (uint8_t)(MESH_FWID>>8);
            devdata[13] = (uint8_t)(MESH_FWID&0xFF);
            devdata[14] = 0x01;
            adv_elem[num_elem].advert_type  = BTM_BLE_ADVERT_TYPE_FLAG;
            adv_elem[num_elem].len          = sizeof(uint8_t);
            adv_elem[num_elem].p_data       = &flag;
            num_elem++;
            adv_elem[num_elem].advert_type  = BTM_BLE_ADVERT_TYPE_MANUFACTURER;
            adv_elem[num_elem].len          = 15;
            adv_elem[num_elem].p_data       = devdata;
            num_elem++;
        }

        // advStartAndonService(num_elem, adv_elem); 
        wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);

        
        num_elem = 0;
        adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
        //adv_elem[num_elem].len = (uint16_t)strlen((const char *)mywiced_bt_cfg_settings.device_name);
        //adv_elem[num_elem].p_data = mywiced_bt_cfg_settings.device_name;
        adv_elem[num_elem].len = (uint16_t)strlen((const char *)wiced_bt_cfg_settings.device_name);
        adv_elem[num_elem].p_data = wiced_bt_cfg_settings.device_name ;
                
        num_elem++;

        adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
        adv_elem[num_elem].len = 2;
        buf[0] = (uint8_t)wiced_bt_cfg_settings.gatt_cfg.appearance;
        buf[1] = (uint8_t)(wiced_bt_cfg_settings.gatt_cfg.appearance >> 8);
        adv_elem[num_elem].p_data = buf;
        num_elem++;

        LOG_VERBOSE("Start BLE Advertising\n");
        wiced_bt_ble_set_raw_scan_response_data(num_elem, adv_elem);
        if(andon_app_state.run_mode != ANDON_APP_RUN_WAIT_PROVISION){
            wiced_bt_start_advertisements( BTM_BLE_ADVERT_UNDIRECTED_HIGH, BLE_ADDR_RANDOM, NULL );
        }else{
            wiced_bt_start_advertisements( BTM_BLE_ADVERT_UNDIRECTED_LOW, BLE_ADDR_RANDOM, NULL );
        }
    }
    return WICED_FALSE;

}


void AppGetMyMac(wiced_bt_device_address_t addr)
{
    uint8_t *p_data;
    p_data = (uint8_t *)addr;
    for(uint8_t i=0;i<12;i+=2)
    {
        p_data[i/2] =  HexStr2Int8U(mesh_system_id+i+strlen(PRODUCT_CODE)+1);
    }
}


