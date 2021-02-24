//******************************************************************************
//*
//* �� �� �� : 
//* �ļ����� : 
//* ��    �� : zhw/Andon Health CO.LTD
//* ��    �� : V0.0
//* ��    �� : 
//* �����б� : ��.c
//* ������ʷ : ��.c           
//******************************************************************************
#ifndef _MESH_BTN_HANDLER_H_
#define _MESH_BTN_HANDLER_H_

///*****************************************************************************
///*                         �����ļ�˵��
///*****************************************************************************
#include "wiced_hal_gpio.h"
#ifdef CYW20706A2
#include "wiced_bt_app_hal_common.h"
#else
#include "wiced_platform.h"
#endif

#include "wiced_bt_btn_handler.h"

///*****************************************************************************
///*                         �궨����
///*****************************************************************************
#define ENCODER                           (1)



//����˳��
#define BTN_ONOFF_INDEX                       (0)              
#if !ENCODER
    #define BTN_NUM                           (4)
    #define BTN_LIGHTNESS_DOWN_INDEX          (1)
    #define BTN_LIGHTNESS_UP_INDEX            (2)
    #define BTN_RESET_INDEX                   (3)   
    #define BTN_LIGHTNESS_DOWN_VALUE          (1<<BTN_LIGHTNESS_DOWN_INDEX)
    #define BTN_LIGHTNESS_UP_VALUE            (1<<BTN_LIGHTNESS_UP_INDEX)
#else
    #define BTN_NUM                           (2)
    #define BTN_RESET_INDEX                   (1)    
    #define BTN_ENCODER_MAX_CNT               200     
#endif

#define BTN_PRESSED_MAX_CNT                   5000
#define BTN_RELEASE_MAX_CNT                   5000

#define BTN_ONOFF_VALUE                       (1<<BTN_ONOFF_INDEX)
#define BTN_RESET_UP_VALUE                    (1<<BTN_RESET_INDEX)
#define BTN_MODE_PAIRED_VALUE                 0x00
#define BTN_MODE_MASK_VALUE                   0x07     

#if !ENCODER
    #define BTN_RELEASE_MASK              ( BTN_ONOFF_VALUE             \
                                            + BTN_LIGHTNESS_DOWN_VALUE  \
                                            + BTN_LIGHTNESS_UP_VALUE
                                            + BTN_RESET_UP_VALUE )
#else
    #define BTN_RELEASE_MASK              ( BTN_ONOFF_VALUE \
                                            + BTN_RESET_UP_VALUE)
#endif

#define BTN_ENCODER_MAX_CNT                  200              //
#define BTN_ENCODER_FACTOR                   (20)
#define BTN_ENCODER_MIN_STEP                 5                //

///*****************************************************************************
///*                         Strcut�ṹ����������
///*****************************************************************************
typedef struct
{
    uint32_t                 btn_state_level;              //��ǰ����״̬�����Ű����������̶��仯�������˿�״̬--����ʶ��״̬
    uint32_t                 btn_state_level0;             //�ϴΰ���״̬������ʾ�˿�״̬
    uint32_t                 btn_state_level1;             //���ϴΰ���״̬������ʾ�˿�״̬
    uint32_t                 btn_pairpin_level;            //��Զ˿ڵ�״̬
    uint32_t                 btn_up;                       //����̧��
    uint32_t                 btn_down;                     //��������
    uint16_t                 btn_idle_count;               //���а���̧�����
    uint16_t                 btn_press_count[BTN_NUM];     //����������ס����
    wiced_bool_t             btn_pressed;                  //�Ƿ��а���������
    wiced_bool_t             btn_ghost;                    //�Ƿ���keyscan�쳣
#if ENCODER 
    int16_t                  btn_encoder_count;            //��ȡ�ı������  ˳ʱ��Ϊ������ʱ��Ϊ��
#endif
} mesh_btn_handler_t;

///*****************************************************************************
///*                         ����������
///*****************************************************************************
void mesh_btn_init(void);
void btnKeyScanStart(void);
void wiced_btn_scan(void);
///*****************************************************************************
///*                         �ⲿȫ�ֱ���������
///*****************************************************************************
extern mesh_btn_handler_t btn_state;   // Application state



#endif
