//******************************************************************************
//*
//* �� �� �� : 
//* �ļ����� :        
//* ��    �� : ����/Andon Health CO.Ltd
//* ��    �� : V0.0
//* ��    �� : 
//******************************************************************************
#ifndef __DISPLAY_H__
#define __DISPLAY_H__
///*****************************************************************************
///*                         �����ļ�˵��
///*****************************************************************************

///*****************************************************************************
///*                         �궨����
///*****************************************************************************
#define ANDON_APP_DISPLAY_NORMAL                    0x0000           //��ͨģʽ
//#define ANDON_APP_DISPLAY_BTN_PRESS                 0x0001           //�������»���ת������
#define ANDON_APP_DISPLAY_BTN_RESET3S               0x0002           //����RESET��3s
#define ANDON_APP_DISPLAY_BTN_RESET10S              0x0003           //����RESET��10s
#define ANDON_APP_DISPLAY_FLASH                     0x0004           //����ǰ�������
#define ANDON_APP_DISPLAY_CONFIG                    0x0005           //��������ģʽ
#define ANDON_APP_DISPLAY_BTN_PRESS_NORMAL          0X0006           //��ͨģʽ�°������»���ת������
#define ANDON_APP_DISPLAY_BTN_PRESS_CONFIG          0X0007           //����ģʽ�°������»���ת������
#define ANDON_APP_DISPLAY_LOWPOWER                  0x00FF           //�͵�ѹ
#define ANDON_APP_DISPLAY_PAIR_DOING                0x0100           //�����
#define ANDON_APP_DISPLAY_PAIR_SUCCESS              0x0101           //��Գɹ�
#define ANDON_APP_DISPLAY_PAIR_FAILED               0x0102           //���ʧ��
#define ANDON_APP_DISPLAY_PROVISION_DOING           0x0200           //provisioning
#define ANDON_APP_DISPLAY_PROVISION_DONE            0x0201           //provisioned
#define ANDON_APP_DISPLAY_TEST_DOING                0xFF00           //���Թ�����
#define ANDON_APP_DISPLAY_TEST_BLEOK                0xFF01           //ble����ͨ��
#define ANDON_APP_DISPLAY_TEST_BLEFAILED            0xFF02           //ble����ͨ��
#define ANDON_APP_DISPLAY_TEST_MODEFAILED           0xFF03           //�ͺ�У��δͨ��
#define ANDON_APP_DISPLAY_TEST_DIDFAILED            0xFF04           //DIDУ��δͨ��
#define ANDON_APP_DISPLAY_TEST_UPGRADEOK            0xFF05           //��Ϲ�װ�������            




///*****************************************************************************
///*                         Strcut�ṹ����������
///*****************************************************************************

///*****************************************************************************
///*                         ����������
///*****************************************************************************
void DisplayRefresh(wiced_bool_t is_provison);
void DisplayInit(void);
void DisplayDeinit(void);
///*****************************************************************************
///*                         �ⲿȫ�ֳ���������
///*****************************************************************************


#endif