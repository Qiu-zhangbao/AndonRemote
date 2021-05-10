@echo off
@echo WICED envirment initialize...

rem 环境定义
rem @set SDK_VERSION=WICED-6.4.0.1002
@set SDK_VERSION1=WICED-Studio-6.4-1113
@set TARGET=20735-B1_Bluetooth
@set SDK_PATH=D:\Cypress\6.4.0.1023\SDK\%SDK_VERSION1%\%TARGET%
@set APP=AndonRemote
rem 环境定义结束

@echo SDK: %SDK_VERSION%
@echo TARGET: %TARGET%
rem @set PATH=%PATH%;D:\SDK\%SDK_VERSION%\%SDK_VERSION1%\%TARGET%\
@set PATH=%PATH%;%SDK_PATH%
@pushd .
rem @cd D:\SDK\%SDK_VERSION%\%SDK_VERSION1%\%TARGET%\
@cd %SDK_PATH%
@echo on
@echo.
@echo Begin Build
@echo.
@make clean
@REM @set ymd=%date:~0,4%%date:~5,2%%date:~8,2%
@REM set mkdir "%ymd%"
@REM rd /s /q D:\Cypress\6.4.0.1014\SDK\%SDK_VERSION1%\%TARGET%\build\%%Light-%d%%%
@REM @make Andon.Light-CYW920735Q60EVB_01 _DEBUG=LOGLEVEL_VERBOSE LIGHT=1 USE_REMOTE_PROVISION=1 UART=com23 download%*
@REM @make Andon.Light-CYW920735Q60EVB_01 _DEBUG=LOGLEVEL_DEBUG LIGHT=2 USE_REMOTE_PROVISION=1 PRESS_TEST=1 UART=com9 download%*
@make Andon.AndonRemote-CYW920735Q60EVB_01 LOW_POWER_NODE=1 ANDON_TEST=0 RELEASE=0 ANDON_BEEP=0 UART=com22 download%*
@REM @make Andon.Remote-CYW920735Q60EVB_01 LOW_POWER_NODE=1 USE_REMOTE_PROVISION=1 build%*
@REM @make Andon.Light-CYW920735Q60EVB_01 _DEBUG=LOGLEVEL_INFO LIGHT=1 USE_REMOTE_PROVISION=1 UART=com4 download%*
@REM @make Andon.Light-CYW920735Q60EVB_01 _DEBUG=LOGLEVEL_WARNNING LIGHT=2 USE_REMOTE_PROVISION=1 UART=com7 download%*
@REM @make Andon.Light-CYW920735Q60EVB_01 LIGHT=2 USE_REMOTE_PROVISION=1 UART=com7 download%*
@cd %SDK_PATH%\build\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%
@..\..\hex2bin %APP%-%date:~0,4%%date:~5,2%%date:~8,2%.hex
@echo %date%%time%
@REM @make projects.WICED.dev_kit %*
@popd

@echo off 
if EXIST ".\target" (
    rd /s/q ".\target")
mkdir ".\target"
@echo on

@copy "%SDK_PATH%\build\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%.hex" ".\target"
@copy "%SDK_PATH%\build\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%.bin" ".\target"
@copy "%SDK_PATH%\build\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%.ota.hex" ".\target"
@copy "%SDK_PATH%\build\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%\%APP%-%date:~0,4%%date:~5,2%%date:~8,2%.ota.bin" ".\target"
@echo %date%%time%

