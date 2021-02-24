#
# Copyright 2018, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor 
#  Corporation. All rights reserved. This software, including source code, documentation and  related 
# materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its 
#  subsidiaries ("Cypress") and is protected by and subject to worldwide patent protection  
# (United States and foreign), United States copyright laws and international treaty provisions. 
# Therefore, you may use this Software only as provided in the license agreement accompanying the 
# software package from which you obtained this Software ("EULA"). If no EULA applies, Cypress 
# hereby grants you a personal, nonexclusive, non-transferable license to  copy, modify, and 
# compile the Software source code solely for use in connection with Cypress's  integrated circuit 
# products. Any reproduction, modification, translation, compilation,  or representation of this 
# Software except as specified above is prohibited without the express written permission of 
# Cypress. Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO  WARRANTY OF ANY KIND, EXPRESS 
# OR IMPLIED, INCLUDING,  BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY 
# AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to 
# the Software without notice. Cypress does not assume any liability arising out of the application 
# or use of the Software or any product or circuit  described in the Software. Cypress does 
# not authorize its products for use in any products where a malfunction or failure of the 
# Cypress product may reasonably be expected to result  in significant property damage, injury 
# or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the 
#  manufacturer of such system or application assumes  all risk of such use and in doing so agrees 
# to indemnify Cypress against all liability.
#

NAME := mesh_onoff_client_test

# Mesh debug: enable (or pass via make target) to use debug/trace mesh libraries
# NOTE:  20706 limited to only 1 of mesh_models or mesh_core debug libraries
#        - do not enable both simultaneously for 20706
# MESH_MODELS_DEBUG := 1
# MESH_CORE_DEBUG := 1

C_FLAGS += -DWICED_OTA_VERSION_1
C_FLAGS += -DENABLE_SELF_CONFIG_MODEL
C_FLAGS += -DSFLASH_SIZE_8M_BITS
########################################################################
# Add Application sources here.
########################################################################
APP_SRC = src/Andon_App.c
APP_SRC += src/log.c
APP_SRC += src/AndonPair.c
APP_SRC += src/AndonCmd.c
APP_SRC += src/app_config.c
APP_SRC += src/mesh_btn_handler.c
APP_SRC += src/mesh_app_self_config.c
#APP_SRC += src/mesh_init_node.c
APP_SRC += src/adv_pack.c
APP_SRC += src/mesh_vendor.c
APP_SRC += src/display.c
APP_SRC += src/AES.c
APP_SRC += src/handle_key.c
APP_SRC += src/mesh_application.c
APP_SRC += src/mesh_app_provision_server.c
APP_SRC += src/mesh_app_hci.c
APP_SRC += src/mesh_app_gatt.c
APP_SRC += src/raw_flash.c
APP_SRC += src/storage.c
APP_SRC += src/wiced_bt_cfg.c
APP_SRC += src/mylib.c
APP_SRC += src/DH.c
APP_SRC += src/wyzebase64.c
APP_SRC += src/xxtea_F.c
APP_SRC += src/andon_server.c
APP_SRC += src/WyzeService.c
APP_SRC += src/tooling.c
APP_SRC += src/wiced_platform_pin_config.c

ifeq ($(ANDON_BEEP),1) 
C_FLAGS += -DANDON_BEEP
APP_SRC += src/Beep.c
endif

########################################################################
# Component(s) needed
########################################################################
#$(NAME)_COMPONENTS := mesh_app_lib.a
$(NAME)_COMPONENTS := fw_upgrade_lib.a
$(NAME)_COMPONENTS += factory_config_lib.a

ifeq ($(MESH_MODELS_DEBUG),1)
$(NAME)_COMPONENTS += mesh_models_lib-d.a
else
$(NAME)_COMPONENTS += mesh_models_lib.a
endif
ifeq ($(MESH_CORE_DEBUG),1)
$(NAME)_COMPONENTS += mesh_core_lib-d.a
else
#$(NAME)_COMPONENTS += mesh_core_lib_getway.a
#$(NAME)_COMPONENTS += mesh_core_lib_0310_lpn.a  #Johnson改的LPN的库
#$(NAME)_COMPONENTS += mesh_core_lib.a            #最新patch的库
#$(NAME)_COMPONENTS += mesh_core_lib_lpn_20200801.a         #0723 修复LPN不休眠
$(NAME)_COMPONENTS += mesh_core_lib_20200831.a    #0831 测试开关发消息灯收不到的问题
#$(NAME)_COMPONENTS += mesh_core_lib_0901.a  #0901 测试开关发消息灯收不到的问题
#$(NAME)_COMPONENTS += mesh_core_lib_1123.a
endif

ifeq ($(BLD),A_20719B0)
APP_PATCHES_AND_LIBS := multi_adv_patch_lib.a
else ifeq ($(CHIP),20703)
APP_PATCHES_AND_LIBS := rtc_lib.a
APP_PATCHES_AND_LIBS += wiced_bt_mesh.a
endif

########################################################################
# C flags
########################################################################
# define this for 2 chip solution or for testing over WICED HCI
#C_FLAGS += -DHCI_CONTROL

# Warning! Low Power Node will not be accessable after provisioning if network has no Friend nodes.
# Value of LOW_POWER_NODE defines mode. It can be normal node(0), LPN(1) or friend(2)
ifdef LOW_POWER_NODE
C_FLAGS += -DLOW_POWER_NODE=$(LOW_POWER_NODE)
else
C_FLAGS += -DLOW_POWER_NODE=0
endif
ifeq ($(LOW_POWER_NODE), 1)
APP_SRC += src/mesh_app_lpn.c
endif

ifeq ($(RELEASE),1)
C_FLAGS += -DRELEASE=1
C_FLAGS += -DCHECK_BATTERY_VALUE=1
else
C_FLAGS += -DRELEASE=0
C_FLAGS += -DCHECK_BATTERY_VALUE=0
endif

ifeq ($(ANDON_TEST),1)
C_FLAGS += -DANDON_TEST=1
else
C_FLAGS += -DANDON_TEST=0
endif

C_FLAGS += -DNODE_UUID_HAS_MAGIC_NUM

ifeq ($(USE_REMOTE_PROVISION),1)
C_FLAGS += -DUSE_REMOTE_PROVISION  
C_FLAGS += -DREMOTE_PROVISION_SERVER_SUPPORTED
#C_FLAGS += -DREMOTE_PROVISION_OVER_GATT_SUPPORTED  #确定手机范围内的设备是使用GATT入网还是通过ADV通过代理节点入网 加此宏是通过GATT入网
endif

ifeq ($(RELEASE),1)
C_FLAGS += -D_ANDONDEBUG=1
C_FLAGS += -DLOGLEVEL=LOGLEVEL_INFO
# C_FLAGS += -DLOGLEVEL=LOGLEVEL_DEBUG
# C_FLAGS += -DWICED_BT_TRACE_ENABLE
else
C_FLAGS += -D_ANDONDEBUG=1
C_FLAGS += -DLOGLEVEL=LOGLEVEL_DEBUG
C_FLAGS += -DWICED_BT_TRACE_ENABLE
endif


