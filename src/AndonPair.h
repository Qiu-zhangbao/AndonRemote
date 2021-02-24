//******************************************************************************
//*
//* 文 件 名 : AndonPair.h
//* 文件描述 :        
//* 作    者 : 张威/Andon Health CO.Ltd
//* 版    本 : V0.0
//* 日    期 : 
//******************************************************************************
#ifndef _ANDONPAIR_H__
#define _ANDONPAIR_H__
///*****************************************************************************
///*                         包含文件说明
///*****************************************************************************
#include "spar_utils.h"

///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
//状态
#define ANDONPAIR_PAIR_IDLE                     0
#define ANDONPAIR_PAIR_DOING                    1
#define ANDONPAIR_PAIR_DONE                     2
#define ANDONPAIR_PAIR_FAILED                   3
#define ANDONPAIR_PAIR_WAITPROING               4
#define ANDONPAIR_PAIR_PROED                    5

//结果
#define ANDONPAIR_PAIR_SUCCESS                  0
#define ANDONPAIR_PAIR_FINISH                   1
#define ANDONPAIR_PAIR_FAILED_TIMEOUT           0xFF

#define ANDONPAIR_PAIRTIMELENGTH                3600  //单位s


///*****************************************************************************
///*                         数据类型重定义去
///*****************************************************************************
typedef void(AndonPair_Done_cback_t)(uint8_t);


///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
void AndonPair_Confirm(void);
void AndonPair_Paired(void);
void AndonPair_Next(int16_t delta_num);
void AndonPair_Stop(void);
void AndonPair_Start(void);
void AndonPair_Init(AndonPair_Done_cback_t *cback);
void AndonPair_DeInit(wiced_bool_t);
void AndonPair_Unband(void);
void AndonPair_ResetFactor1(void);
///*****************************************************************************
///*                         外部全局常量声明区
///*****************************************************************************

///*****************************************************************************
///*                         外部全局变量声明区
///*****************************************************************************

#endif