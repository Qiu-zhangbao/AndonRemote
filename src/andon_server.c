//******************************************************************************
//*
//* 文 件 名 : andon_server.c
//* 文件描述 : Andon自定义协议GATT服务数据处理       
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
#include <stdio.h>
#include "mylib.h"
#include "wiced_hal_rand.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "andon_server.h"
#include "log.h"
#include "wiced_memory.h"
#include "app_config.h"
#include "mesh_application.h"
#include "storage.h"
#include "DH.h"
#include "xxtea_F.h"
#include "wyzebase64.h"

#ifdef DEF_ANDON_GATT
///*****************************************************************************
///*                         宏定义区
///*****************************************************************************
#define PUBLIC_KEYS_P                   0xffffffc5
#define PUBLIC_KEYS_G                   5

//接收到App指令处理结果
#define  andonREPLYSUCCESS              0
#define  andonREPLYOPCODEFAILED         1                   //操作码异常
#define  andonREPLYMEMORYFAILED         2                   //未申请到内存
#define  andonREPLYACTIONFAILED         3                   //执行动作异常

#define _countof(array)                 (sizeof(array) / sizeof(array[0]))

#define ACTION_GET                 (0x01)
#define ACTION_SET                 (0x02)
#define ACTION_DELTA               (0x03)
#define ACTION_TOGGLE              (0x04)
#define ACTION_START               (0x05)
#define ACTION_STATUS              (0x06)
#define ACTION_FLASH               (0x07)
///*****************************************************************************
///*                         Strcut结构变量定义区
///*****************************************************************************

typedef struct gattToMeshCno_t
{
    union{
        struct{
            uint8_t gattcno;
            uint8_t gattAction;
            uint8_t meshcno;
            uint8_t meshAction;
        };
        struct{
            uint16_t hash;
            uint16_t hash1;
        };
    };
    uint32_t saveTime;
} gattToMeshCno;

typedef struct DF_KEY_CONTENT
{
    unsigned int random_key;
    unsigned int exchange_key;
} st_DF_KEY_CONTENT;

typedef struct packagereply_t
{
    uint8_t *p_data;                 //数据处理后返回内存的存储地址，使用后应释放内存 NULL表示无数据
    uint16_t pack_len;               //数据包长度
    uint16_t result;                 //处理结果
} packageReply;

// typedef packageReply (*VendorAction)(uint8_t *p_data, uint16_t data_len);

typedef packageReply (*VendorActionGet)(uint8_t *p_data, uint16_t data_len);
typedef packageReply (*VendorActionSet)(uint8_t *p_data, uint16_t data_len);
typedef packageReply (*VendorActionDelta)(uint8_t *p_data, uint16_t data_len);
typedef packageReply (*VendorActionToggle)(uint8_t *p_data, uint16_t data_len);
typedef packageReply (*VendorActionStart)(uint8_t *p_data, uint16_t data_len);
typedef packageReply (*VendorActionStatus)(uint8_t *p_data, uint16_t data_len);

typedef struct resource_t
{
    uint8_t ResourceId;
    // VendorAction Get;
    // VendorAction Set;
    // VendorAction Delta;
    // VendorAction Toggle;
    // VendorAction Start;
    // VendorAction Status;
    // VendorAction Flash;
    VendorActionGet Get;
    VendorActionSet Set;
    VendorActionDelta Delta;
    VendorActionToggle Toggle;
    VendorActionStart Start;
    VendorActionStatus Status;
    VendorActionStatus Flash;
} Resource;

///*****************************************************************************
///*                         函数声明区
///*****************************************************************************
static packageReply  myOnOffFlash(uint8_t *p_data, uint16_t len);
static packageReply  myGetTokenInfo(uint8_t *p_data, uint16_t len);
static packageReply  myGetDeivceInfo(uint8_t *p_data, uint16_t len);
static packageReply  mySetDeivceBind(uint8_t *p_data, uint16_t len);
static packageReply  myGetexchangeKey(uint8_t *p_data, uint16_t len);


extern uint8_t Auth(uint8_t *data, uint16_t len);
extern void obfs(uint8_t *data, uint16_t len);
///*****************************************************************************
///*                         常量定义区
///*****************************************************************************
const Resource myresource_list[] = {
    {.ResourceId = 0x01,
     .Flash = myOnOffFlash,},
    {.ResourceId = 0x09,
     .Get = myGetTokenInfo},  
    {.ResourceId = 0x30,
     .Get = myGetDeivceInfo,},
    {.ResourceId = 0x3D,
     .Set = mySetDeivceBind,}
};

const Resource myresourceNoEncrypt_list[] = {
    {.ResourceId = 0x3F,
     .Get = myGetexchangeKey,}
};
///*****************************************************************************
///*                         外部全局变量定义区
///*****************************************************************************

///*****************************************************************************
///*                         文件内部全局变量定义区
///*****************************************************************************
static uint16_t andonServiceConfigDescriptor = 0;
static uint8_t  encryptkey[9] = {0};
static uint16_t myconn_id     = 0;
///*****************************************************************************
///*                         函数实现区
///*****************************************************************************
//*****************************************************************************
// 函数名称: GetTokenInfo
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static packageReply  myOnOffFlash(uint8_t *p_data, uint16_t len)
{
    extern void LightFlash(uint16_t cycle, uint16_t times);
    extern int32_t transt_to_period(uint8_t t);

    packageReply reply;
    int16_t flash_timers = 3;
    int16_t flash_cycle = 60;
    
    reply.p_data = NULL;
    reply.pack_len = 0;
    
    if(p_data[2]&0x80)
    {
        reply.result = andonREPLYOPCODEFAILED;
        return reply;
    }

    // for (int i = 0; i < data_len; i++)
    // {
    //     WICED_BT_TRACE("%02x ", p_data[i]);
    // }
    // WICED_BT_TRACE("\n");

    if (len > 4)
    {
        flash_cycle = p_data[4];
        if((flash_cycle > 0xBF) || (0==flash_cycle))
        {
            flash_cycle = 60;
        }
        else
        {
            flash_cycle = transt_to_period(flash_cycle);
        }
    }
    if (len > 5)
    {
        flash_timers = p_data[5];
    }
    reply.p_data = (uint8_t *)wiced_bt_get_buffer(6);
    reply.p_data[0] = 0;
    reply.p_data[1] = p_data[1];
    reply.p_data[2] = p_data[2]|0x80;
    reply.p_data[3] = p_data[3];
    reply.p_data[4] = 0;
    reply.p_data[5] = 0;
    LightFlash(flash_cycle, flash_timers);
    reply.result = andonREPLYSUCCESS;
    reply.pack_len = 6;
    
    return reply;
}
//*****************************************************************************
// 函数名称: GetTokenInfo
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static packageReply  myGetTokenInfo(uint8_t *p_data, uint16_t data_len)
{
    packageReply reply;
    
    reply.p_data = NULL;
    reply.pack_len = 0;
    if(p_data[2]&0x80)
    {
        reply.result = andonREPLYOPCODEFAILED;
        return reply;
    }
    reply.p_data = (uint8_t *)wiced_bt_get_buffer(FLASH_XXTEA_SN_LEN+4);
    if(reply.p_data == NULL)
    {
        reply.result = andonREPLYMEMORYFAILED;
        return reply;
    }
    reply.p_data[0] = 0;
    reply.p_data[1] = p_data[1];
    reply.p_data[2] = (p_data[2]|0x80);
    reply.p_data[3] = p_data[3];
    reply.pack_len = storage_read_sn( reply.p_data+4 );
    obfs(reply.p_data+3,reply.pack_len+1);
    reply.pack_len += 4;
    reply.result = andonREPLYSUCCESS;
    return reply;
}
//*****************************************************************************
// 函数名称: GetDeivceInfo
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static packageReply  myGetDeivceInfo(uint8_t *p_data, uint16_t data_len)
{
    packageReply reply;

    reply.p_data = NULL;
    reply.pack_len = 0;
    if(p_data[2]&0x80)
    {
        reply.result = andonREPLYOPCODEFAILED;
        return reply;
    }
    reply.p_data = (uint8_t *)wiced_bt_get_buffer(12);
    if(reply.p_data == NULL)
    {
        reply.result = andonREPLYMEMORYFAILED;
        return reply;
    }
    reply.p_data[0] = 0;
    reply.p_data[1] = p_data[1];
    reply.p_data[2] = p_data[2]|0x80;
    reply.p_data[3] = p_data[3];
    reply.p_data[4] = (MESH_CID>>8);
    reply.p_data[5] = (MESH_CID&0xFF);
    reply.p_data[6] = (MESH_PID>>8);
    reply.p_data[7] = (MESH_PID&0xFF);
    reply.p_data[8] = (MESH_VID>>8);
    reply.p_data[9] = (MESH_VID&0xFF);
    reply.p_data[10] = (MESH_FWID>>8)&0xFF;
    reply.p_data[11] = (MESH_FWID&0xFF);
    reply.pack_len = 12;
    reply.result = andonREPLYSUCCESS;
    return reply;
}

//*****************************************************************************
// 函数名称: AndonServiceSetBind
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
packageReply myAndonServiceSetBind(uint8_t *p_data, uint8_t len)
{
    packageReply reply;
    uint8_t tmpBuffer[150] = {0};
    uint16_t templen;
    uint8_t *devicetoken;
    uint32_t randomdev;

    reply.p_data = NULL;
    reply.pack_len = 0;

    devicetoken = (uint8_t *)wiced_bt_get_buffer(FLASH_XXTEA_SN_LEN+1); 
    if(devicetoken == NULL)
    {
        reply.result = andonREPLYMEMORYFAILED;
        return reply;
    }
    
    storage_read_sn(devicetoken);
    devicetoken[FLASH_XXTEA_SN_LEN] = '\0';
    if(p_data[0] == 0)
    {   
        uint8_t *encryptdata;
        size_t  encryptlen;

        randomdev = wiced_hal_rand_gen_num();
        devicetoken[FLASH_XXTEA_ID_LEN] = '\0';  //devicetoken由DID+KEY构成，DID在前且未字符串，此处为DID增加结束符
        randomdev = randomdev&0x7FFFFFFF;
        // randomdev = 0;
        mylib_sprintf(tmpBuffer, "{\"device_id\":\"%s\",\"timestamp\":\"%d\"}", devicetoken, randomdev);
        // LOG_DEBUG("%s \n",tmpBuffer);
        wiced_bt_free_buffer(devicetoken);
        templen = strlen(tmpBuffer);
        // LOG_DEBUG("BindToken1: %s \n",p_data+2);
        // WICED_BT_TRACE_ARRAY(p_data+2,16,"BindToken1: len = %d\n",templen);
        encryptdata = WYZE_F_xxtea_encrypt(tmpBuffer, templen, p_data+2, &encryptlen, WICED_TRUE);
        if(NULL == encryptdata)
        {
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        // WICED_BT_TRACE_ARRAY(p_data+2,16,"BindToken1: len = %d\n",templen);
        uint8_t *base64_data = WYZE_base64_encode(encryptdata, encryptlen);
        if(base64_data == NULL)
        {
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        templen = strlen(base64_data);
        reply.p_data = (uint8_t *)wiced_bt_get_buffer(templen+2); 
        if(reply.p_data == NULL)
        {
            wiced_bt_free_buffer(base64_data);
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        reply.p_data[1] = templen&0xff;
        // LOG_DEBUG("deviceToken1 len=%d v=%s \n", templen, base64_data);
        memcpy(reply.p_data+2, base64_data, templen);
        wiced_bt_free_buffer(base64_data);
        reply.p_data[0] = 2;
        reply.pack_len = templen+2;
    }
    else if(p_data[0] == 1)
    {
        uint8_t *encryptdata,*decryptdata;
        size_t  encryptlen,decryptlen;
        uint8_t *base64_data, *base64_key;

        encryptdata = wiced_bt_get_buffer(len);
        memset(encryptdata, 0, len);
        memcpy(encryptdata, p_data+2, len-2);
        LOG_DEBUG("encryptBindTokenB64: %s\n",encryptdata);
        base64_data  = WYZE_base64_decode(encryptdata, &templen);
        if(base64_data == NULL)
        {
            LOG_DEBUG("base64_data: no mem!!!\n");
            wiced_bt_free_buffer(devicetoken);
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        //devicetoken由DID+KEY构成，DID在前且未字符串，KEY在后
        wiced_bt_free_buffer(encryptdata);
        LOG_DEBUG("encryptBindToken: %s\n",base64_data);
        WICED_BT_TRACE_ARRAY(base64_data,templen,"encryptBindTokenHex templen=%d\n",templen);
        WICED_BT_TRACE_ARRAY(devicetoken+FLASH_XXTEA_KEY_OFFSET,FLASH_XXTEA_KEY_LEN,"encryptKeyHex");
        base64_key = WYZE_base64_encode(devicetoken+FLASH_XXTEA_KEY_OFFSET, FLASH_XXTEA_KEY_LEN);
        if(base64_key == NULL)
        {
            LOG_DEBUG("base64_key: no mem!!!\n");
            wiced_bt_free_buffer(devicetoken);
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        decryptdata = WYZE_F_xxtea_decrypt(base64_data,templen,base64_key, &decryptlen, WICED_TRUE);
        wiced_bt_free_buffer(base64_data);
        wiced_bt_free_buffer(base64_key);
        if(NULL == decryptdata)
        {
            LOG_DEBUG("decryptdata: no mem!!!\n");
            wiced_bt_free_buffer(devicetoken);
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        //保存bindkey失败 将bindkey清零
        if(WICED_FALSE == StoreBindKey(decryptdata,decryptlen))
        {
            memset(decryptdata,0,decryptlen);
        }
        WICED_BT_TRACE_ARRAY(decryptdata,decryptlen,"BindTokenHex len = %d ",decryptlen);
        randomdev = wiced_hal_rand_gen_num();
        randomdev = randomdev&0x7FFFFFFF; 
        devicetoken[FLASH_XXTEA_ID_LEN] = '\0';  //devicetoken由DID+KEY构成，DID在前且未字符串，此处为DID增加结束符
        mylib_sprintf(tmpBuffer, "{\"device_id\":\"%s\",\"timestamp\":\"%d\"}", devicetoken, randomdev);
        wiced_bt_free_buffer(devicetoken);
        templen = strlen(tmpBuffer);
        LOG_DEBUG("ConfirmKey: %s\n",tmpBuffer);
        encryptdata = WYZE_F_xxtea_encrypt(tmpBuffer,templen,decryptdata, &encryptlen, WICED_TRUE);
        wiced_bt_free_buffer(decryptdata);
        if(NULL == encryptdata)
        {
            LOG_DEBUG("encryptdata: no mem!!!\n");
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        base64_data = WYZE_base64_encode(encryptdata, encryptlen);
        wiced_bt_free_buffer(encryptdata);
        if(base64_data == NULL)
        {
            LOG_DEBUG("base64_data: no mem!!!\n");
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        templen = strlen(base64_data);
        reply.p_data = (uint8_t *)wiced_bt_get_buffer(templen+2); 
        if(reply.p_data == NULL)
        {
            LOG_DEBUG("reply: no mem!!!\n");
            wiced_bt_free_buffer(base64_data);
            reply.result = andonREPLYMEMORYFAILED;
            return reply;
        }
        reply.p_data[1] = templen&0xFF;
        LOG_DEBUG("ConfirmKeyEncryptB64: %s\n",base64_data);
        memcpy(reply.p_data+2, base64_data, templen);
        wiced_bt_free_buffer(base64_data);
        reply.pack_len = templen+2;
        reply.p_data[0] = 3;
    }
    else if(p_data[0] == 0xFF)
    {
        StoreBindKey(NULL,0);
        storageBindkey.bindflag = WICED_FALSE;
        reply.p_data = (uint8_t *)wiced_bt_get_buffer(2); 
        reply.pack_len = 2;
        reply.p_data[0] = 0xFF;
        reply.p_data[1] = 0;
    }
    reply.result = andonREPLYSUCCESS;
    return reply;
}

//*****************************************************************************
// 函数名称: SetDeivceBind
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static packageReply mySetDeivceBind(uint8_t *p_data, uint16_t data_len)
{
    packageReply reply;
    packageReply reply1;

    reply.p_data = NULL;
    reply.pack_len = 0;

    if(p_data[2]&0x80)
    {
        reply.result = andonREPLYOPCODEFAILED;
        return reply;
    }

    if(data_len < 2)
    {
        reply.result = andonREPLYACTIONFAILED;
        return reply;
    }

    reply1 = myAndonServiceSetBind(p_data+4,data_len-4);
    if(reply1.result != andonREPLYSUCCESS){
        // reply.result = reply1.result;
        reply1.pack_len = 0;
        // return reply;
    }else{
        reply.pack_len = reply1.pack_len+4;
        reply.p_data = (uint8_t *)wiced_bt_get_buffer(reply.pack_len); 
        memcpy(reply.p_data+4, reply1.p_data,reply1. pack_len);
        // WICED_BT_TRACE_ARRAY(reply.p_data,reply.pack_len,"Anond len=%d Binddata: \n",reply.pack_len);
        wiced_bt_free_buffer(reply1.p_data);
    }
    reply.p_data[0] = 0;
    reply.p_data[1] = p_data[1];
    reply.p_data[2] = p_data[2]|0x80;
    reply.p_data[3] = p_data[3];
    reply.result = andonREPLYSUCCESS;
    return reply;
}

//*****************************************************************************
// 函数名称: AndonServiceSetEncryptKey
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
static packageReply myGetexchangeKey(uint8_t *p_data, uint16_t len)
{
    extern void wiced_hal_wdog_disable(void);
    st_DF_KEY_CONTENT KeyBody={0};
    uint32_t secretkey ;
    packageReply Reply ;

    if(p_data[2]&0x80)
    {
        Reply.result = andonREPLYOPCODEFAILED;
        return Reply;
    }
    for (int i = 0; i < 4; i++) {
        KeyBody.exchange_key = KeyBody.exchange_key  << 8;
        KeyBody.exchange_key = KeyBody.exchange_key + p_data[i+4];
    }
    LOG_DEBUG("rec ex_key  %x\n",KeyBody.exchange_key);
    KeyBody.random_key = wiced_hal_rand_gen_num();
    secretkey = DHSecretKeyGenerate(KeyBody.random_key, KeyBody.exchange_key, PUBLIC_KEYS_P);
    mylib_sprintf(encryptkey, "%08x", secretkey);
    LOG_DEBUG("secretkey %s\n",encryptkey);
    KeyBody.exchange_key = DHKeyGenerate(KeyBody.random_key, PUBLIC_KEYS_P, PUBLIC_KEYS_G);
    Reply.pack_len = 0;
    Reply.p_data = (uint8_t *)wiced_bt_get_buffer(8); 
    if(Reply.p_data == NULL)
    {
        Reply.result = andonREPLYMEMORYFAILED;
        return Reply; 
    }
    LOG_DEBUG("sen ex_key  %x\n",KeyBody.exchange_key);
    Reply.p_data[0] = 0;
    Reply.p_data[1] = p_data[1];
    Reply.p_data[2] = p_data[2]|0x80;
    Reply.p_data[3] = p_data[3];
    Reply.p_data[4] = (KeyBody.exchange_key>>24)&0xFF;
    Reply.p_data[5] = (KeyBody.exchange_key>>16)&0xFF;
    Reply.p_data[6] = (KeyBody.exchange_key>>8)&0xFF;
    Reply.p_data[7] = KeyBody.exchange_key&0xFF;
    Reply.pack_len = 8;
    Reply.result = andonREPLYSUCCESS;

    return Reply;
}


//*****************************************************************************
// 函数名称: findResource
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
Resource *myfindResource(uint8_t resource_id)
{
    Resource *p = (Resource *)myresource_list;

    for (int i = 0; i < _countof(myresource_list); i++)
    {
        if (p->ResourceId == resource_id)
        {
            return p;
        }
        p++;
    }
    return NULL;
}
//*****************************************************************************
// 函数名称: findResourceNoEncrypt
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
Resource *myfindResourceNoEncrypt(uint8_t resource_id) 
{
    Resource *p = (Resource *)myresourceNoEncrypt_list;

    for (int i = 0; i < _countof(myresourceNoEncrypt_list); i++)
    {
        if (p->ResourceId == resource_id)
        {
            return p;
        }
        p++;
    }
    return NULL;
}


//*****************************************************************************
// 函数名称: andonServerUnpackMsgNoEncrypt
// 函数描述: 无加密数据传输的数据解包，与加密数据采用不同的函数，以便在未获取秘钥的情况下
//          不处理需使用加密数据传输的指令
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
packageReply andonServerUnpackMsgNoEncrypt(uint8_t *p_data, uint16_t data_len)
{
    packageReply reply;

    Resource *item = myfindResourceNoEncrypt(p_data[3]);
    reply.result = andonREPLYSUCCESS;
    reply.pack_len = 0;
    reply.p_data   = NULL;
    
    if (item)
    {
        LOG_DEBUG("findResourceNoEncrypt\n");
        switch (p_data[2]&0x7F)
        {
            case ACTION_GET:
                if (item->Get)
                {
                    LOG_DEBUG("call Get\n");
                    reply = item->Get(p_data, data_len);
                }
                break;
        }
    }
    return reply;
}

//*****************************************************************************
// 函数名称: andonServerUnpackMsg
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
packageReply andonServerUnpackMsg(uint8_t *p_data, uint16_t data_len)
{
    packageReply reply;
    Resource *item = myfindResource(p_data[3]);

    reply.result = andonREPLYSUCCESS;
    reply.pack_len = 0;
    reply.p_data   = NULL;
    if (item)
    {
        LOG_DEBUG("findResourceEncrypt\n");
        switch (p_data[2]&0x7F)
        {
            case ACTION_GET:
                if (item->Get)
                {
                    LOG_DEBUG("call get\n");
                    reply = item->Get(p_data, data_len);
                }
                break;
            case ACTION_SET:
                if (item->Set)
                {
                    LOG_DEBUG("call set\n");
                    reply = item->Set(p_data, data_len);
                }
                break;
            case ACTION_DELTA:
                break;
            case ACTION_TOGGLE:
                break;
            case ACTION_START:
                break;
            case ACTION_STATUS:
                break;
            case ACTION_FLASH:
                if (item->Flash)
                {
                    LOG_DEBUG("call Flash\n");
                    reply = item->Flash(p_data, data_len);
                    
                }
                break;
        }
    }
    return reply;
}

//*****************************************************************************
// 函数名称: AndonGattSendNotification
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
wiced_bool_t AndonGattSendNotification (uint16_t conn_id, uint16_t val_len, uint8_t *p_val, wiced_bool_t encryptflag)
{
    // if ((andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_NOTIFICATION) || (andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_INDICATION)) 
    {
        if(mesh_app_gatt_is_connected())
        {
            wiced_bt_gatt_status_t result;
            uint8_t *p_data;
            uint16_t templen;

            p_data = NULL;
            templen = val_len+1;
            if(encryptflag)
            {
                templen = (templen%8?((templen>>3)+1)<<3:templen);
            }
            p_data = (uint8_t*)wiced_bt_get_buffer(templen + 5);
            if(p_data == NULL)
            {
                return WICED_FALSE;
            }
            memset(p_data,0,templen+5);
            
            p_data[1] = 0;  //src Gatt直连 恒为0
            p_data[2] = 0;
            p_data[3] = 0;  //dst Gatt直连 恒为0
            p_data[4] = 0;
            
            
            WICED_BT_TRACE_ARRAY(p_val,val_len, "Andon To App Source \n");
            obfs(p_val,val_len);
            p_val[0] = Auth(p_val,val_len);
            p_data[5] = val_len;
            memcpy(p_data+6,p_val,val_len);
            val_len = templen; //增加了1个字节的数据长度
            if(!encryptflag)
            {
                p_data[0] = 0;  //Type Gatt直接交互非加密数据
            }
            else
            {
                uint8_t *output;
                size_t  outlen;
                output = WYZE_F_xxtea_encrypt(p_data+5, val_len, encryptkey, &outlen, WICED_FALSE);
                if(output == NULL)
                {
                    wiced_bt_free_buffer(p_data);
                    return WICED_FALSE;
                }
                val_len = outlen;
                memcpy(p_data+5, output, val_len);
                wiced_bt_free_buffer(output);
                p_data[0] = 2;  //Type Gatt直接交互加密数据
            }
            // LOG_DEBUG("Andon Notification To App\n");
            WICED_BT_TRACE_ARRAY(p_data,val_len+5,"Andon To App EnCode \n");
            if(andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_NOTIFICATION){
                result = wiced_bt_gatt_send_notification(conn_id, HANDLE_ANDON_SERVICE_CHAR_NOTIFY_VAL, val_len+5, p_data);
            }else if(andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_INDICATION){
                result = wiced_bt_gatt_send_indication(conn_id, HANDLE_ANDON_SERVICE_CHAR_NOTIFY_VAL, val_len+5, p_data);
            }
            wiced_bt_free_buffer(p_data);
            if(result == WICED_BT_GATT_SUCCESS)
            {
                return WICED_TRUE;
            }
            else
            {
                return WICED_FALSE;
            }
        }
    }
    return WICED_FALSE;
}
//*****************************************************************************
// 函数名称: AndonServiceSetClientConfiguration
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonServiceSetClientConfiguration(uint16_t client_config)
{
    extern void appUpdataCommpara(void);
    andonServiceConfigDescriptor = client_config;
    if(client_config == 0)
    {
        memset(encryptkey,0,sizeof(encryptkey));
    }else{
        appUpdataCommpara();
    }
    LOG_DEBUG("customer_service_config_descriptor changed: %d\n", andonServiceConfigDescriptor);
}

//*****************************************************************************
// 函数名称: AndonServiceGattDisConnect
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
void AndonServiceGattDisConnect(void)
{
    AndonServiceSetClientConfiguration(0);
    myconn_id = 0;
    memset(encryptkey,0,sizeof(encryptkey));
}

//*****************************************************************************
// 函数名称: AndonServerHandle 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
wiced_bt_gatt_status_t AndonServerHandle(uint16_t conn_id, wiced_bt_gatt_write_t * p_data)
{
    uint8_t *userData;
    uint8_t userDataLen;
    packageReply replyAck;
    
    if(p_data->p_val[0] == 0xFF)  //测试数据，直接返回
    {
        if(andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_NOTIFICATION){
                wiced_bt_gatt_send_notification(conn_id, HANDLE_ANDON_SERVICE_CHAR_NOTIFY_VAL, p_data->val_len, p_data->p_val);
        }else if(andonServiceConfigDescriptor & GATT_CLIENT_CONFIG_INDICATION){
                wiced_bt_gatt_send_indication(conn_id, HANDLE_ANDON_SERVICE_CHAR_NOTIFY_VAL, p_data->val_len, p_data->p_val);
        }
        return WICED_BT_GATT_SUCCESS;
    }

    if (p_data->val_len < 9)
        return WICED_BT_GATT_INVALID_ATTR_LEN;
    

    if(p_data->p_val[0] == 2)     //GATT直接带加密
    {
        uint8_t *output;
        size_t  outlen;
        WICED_BT_TRACE_ARRAY(p_data->p_val,p_data->val_len,"Gatt Data Len = %d ",p_data->val_len);
        WICED_BT_TRACE_ARRAY(encryptkey,sizeof(encryptkey),"encryptkey: ");
        output = WYZE_F_xxtea_decrypt(p_data->p_val + 5, p_data->val_len - 5, encryptkey, &outlen, WICED_FALSE);
        if(output == NULL)
        {
            LOG_DEBUG("WICED_BT_GATT_INTERNAL_ERROR\n");
            return WICED_BT_GATT_INTERNAL_ERROR;
        }
        p_data->val_len = output[0]+5;
        memcpy(p_data->p_val + 5,output+1,outlen);
        WICED_BT_TRACE_ARRAY(p_data->p_val,p_data->val_len,"Gatt Data Len = %d ",p_data->val_len);
        wiced_bt_free_buffer(output);
    }
    else if(p_data->p_val[0] == 0)  
    {
        WICED_BT_TRACE_ARRAY(p_data->p_val,p_data->val_len,"Gatt Data Len = %d ",p_data->val_len);
        memcpy(p_data->p_val + 5,p_data->p_val + 6,p_data->p_val[5]);
        p_data->val_len = p_data->val_len - 1;
    }

    userDataLen = p_data->val_len - 5;
    if (Auth(p_data->p_val + 5, userDataLen) != 0)
        return WICED_BT_GATT_INTERNAL_ERROR;
    obfs(p_data->p_val + 5, userDataLen);
    
    userData = NULL;
    userData = (uint8_t *)wiced_bt_get_buffer(userDataLen);
    if(userData == NULL)
    {
        return WICED_BT_GATT_SUCCESS;
    }
    memcpy(userData,p_data->p_val + 5,userDataLen);
    LOG_VERBOSE("receive gatt data :");
#if LOGLEVEL >= LOGLEVEL_DEBUG
    wiced_bt_trace_array(" ", p_data->p_val,  p_data->val_len);
#endif
    
    if(p_data->p_val[0] == 0)     //GATT直接无加密
    {
        //数据解析
        replyAck = andonServerUnpackMsgNoEncrypt(userData,userDataLen);
        wiced_bt_free_buffer(userData);
        if(replyAck.result == andonREPLYSUCCESS) 
        {
            if(replyAck.p_data != NULL)
            {
                AndonGattSendNotification(conn_id, replyAck.pack_len, replyAck.p_data, WICED_FALSE);
                wiced_bt_free_buffer(replyAck.p_data);
            }
        }
        return WICED_BT_GATT_SUCCESS;

    }
    if(p_data->p_val[0] != 2)     //非GATT直接带加密
    {
        wiced_bt_free_buffer(userData);
        return WICED_BT_GATT_SUCCESS;
    }

    //带加密数据解析
    replyAck = andonServerUnpackMsg(userData,userDataLen);
    if(replyAck.p_data != NULL)
    {
        if(replyAck.result == andonREPLYSUCCESS)
        {
            AndonGattSendNotification(conn_id, replyAck.pack_len, replyAck.p_data, WICED_TRUE);
        }
        wiced_bt_free_buffer(replyAck.p_data);
    }

    wiced_bt_free_buffer(userData);
    return WICED_BT_GATT_SUCCESS;

}
//*****************************************************************************
// 函数名称: AndonServerWriteHandle 
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
wiced_bt_gatt_status_t AndonServerWriteHandle(uint16_t conn_id, wiced_bt_gatt_write_t * p_data)
{
    uint8_t                *attr     = p_data->p_val;
    uint16_t                len      = p_data->val_len;
    uint16_t               temp      = 0;

    myconn_id = conn_id;
    switch (p_data->handle )
    {
        case HANDLE_ANDON_SERVICE_WRITE_VAL:
            return AndonServerHandle(conn_id, p_data);
        break;

        case HANDLE_ANDON_SERVICE_CHAR_CFG_DESC:
            if (p_data->val_len != 2)
            {
                LOG_VERBOSE("customer service config wrong len %d\n", p_data->val_len);
                return WICED_BT_GATT_INVALID_ATTR_LEN;
            }
            temp = p_data->p_val[1];
            temp = (temp<<8) + p_data->p_val[0];
            AndonServiceSetClientConfiguration(temp);
            return WICED_BT_GATT_SUCCESS;
        break;

        default:
            return WICED_BT_GATT_INVALID_HANDLE;
        break;
    }
    return WICED_BT_GATT_INVALID_HANDLE;    
}


//*****************************************************************************
// 函数名称: AndonServerReadHandle
// 函数描述: 
// 函数输入:  
// 函数返回值: 
//*****************************************************************************/
wiced_bt_gatt_status_t AndonServerReadHandle(uint16_t conn_id, wiced_bt_gatt_read_t * p_read_data)
{
    switch (p_read_data->handle)
    {
    case HANDLE_ANDON_SERVICE_CHAR_CFG_DESC:
        if (p_read_data->offset >= 2)
            return WICED_BT_GATT_INVALID_OFFSET;

        if (*p_read_data->p_val_len < 2)
            return WICED_BT_GATT_INVALID_ATTR_LEN;

        if (p_read_data->offset == 1)
        {
            p_read_data->p_val[0] = andonServiceConfigDescriptor >> 8;
            *p_read_data->p_val_len = 1;
        }
        else
        {
            p_read_data->p_val[0] = andonServiceConfigDescriptor & 0xff;
            p_read_data->p_val[1] = andonServiceConfigDescriptor >> 8;
            *p_read_data->p_val_len = 2;
        }
        return WICED_BT_GATT_SUCCESS;
    }
    return WICED_BT_GATT_INVALID_HANDLE;
}



#ifdef DEF_ANDON_OTA
//*****************************************************************************
// 函数名称: AndonServiceUpgradeEncrypt
// 函数描述: 使用者应负责释放返回值使用的内存
// 函数输入:  
// 函数返回值: 返回数据解密后的存储指针
//*****************************************************************************/
void* AndonServiceUpgradeDecrypt(uint8_t *indata, uint16_t inlen, uint16_t *outlen)
{
    uint8_t *decryptdata;
    size_t  outlen1;

    if(indata[0] == 0)  //未加密，直接返回
    {
        decryptdata = (uint8_t *)wiced_bt_get_buffer(inlen-2);
        if(decryptdata == NULL)
        {
            *outlen = 0;
            return NULL;
        }
        *outlen = inlen-2;
        memcpy(decryptdata,indata+2,*outlen);
        return decryptdata;
    }
    if(indata[0] != 2)  //非加密，直接返回
    {
        return NULL;
    }
    decryptdata = WYZE_F_xxtea_decrypt(indata+1, inlen-1, encryptkey, &outlen1, WICED_FALSE);
    if(decryptdata == NULL)
    {
        *outlen = 0;
        return NULL;
    }
    *outlen = decryptdata[0];
    memcpy(decryptdata,decryptdata+1,*outlen);
    return decryptdata;
}

//*****************************************************************************
// 函数名称: AndonServiceUpgradeEncrypt
// 函数描述: 使用者应负责释放返回值使用的内存
// 函数输入:  
// 函数返回值: 返回数据解密后的存储指针
//*****************************************************************************/
void* AndonServiceUpgradeEncrypt(uint8_t *indata, uint16_t inlen, uint16_t *outlen)
{
    uint8_t *output;
    uint8_t *encryptdata;  
    uint16_t temp;
    size_t  outlen1;

    temp = inlen;
    temp = temp+1;
    temp = (temp%8?((temp>>3)+1)<<3:temp);
    //加密算法不会增加数据长度，但加密包标识会占用一个字节长度
    encryptdata = (uint8_t *)wiced_bt_get_buffer(temp+1);
    if(encryptdata == NULL)
    {
        *outlen = 0;
        return NULL;
    }
    memset(encryptdata,0,temp);
    memcpy(encryptdata+2,indata,temp);
    encryptdata[1] = inlen&0xff;
    LOG_DEBUG("Upgrade len=%d\n",inlen);
    output = WYZE_F_xxtea_encrypt(encryptdata+1, temp, encryptkey, &outlen1, WICED_FALSE);
    if(output == NULL)
    {
        *outlen = 0;
        wiced_bt_free_buffer(encryptdata);
        return NULL;
    }
    memcpy(encryptdata+1,output,outlen1);
    wiced_bt_free_buffer(output);
    encryptdata[0] = 2;
    *outlen = outlen1 + 1;
    return encryptdata;
}
#endif
#endif

