#include"wiced_bt_trace.h"
#include"wiced_timer.h"
#include "wiced_hal_gpio.h"
#include "wiced_hal_keyscan.h"
#include "wiced_hal_mia.h"
#include "keyscan.h"
#include "wiced_hal_mia.h"
#include "handle_key.h"

/*******************************************************************************
* Types and Defines
*******************************************************************************/
#define NUM_KEYSCAN_ROWS    3    // Num of Rows in keyscan matrix

#ifndef TESTING_USING_HCI
#define NUM_KEYSCAN_COLS    1   // Num of Cols in keyscan matrix
#else
#define NUM_KEYSCAN_COLS    3   // Num of Cols in keyscan matrix
#endif

#define KEY_TIMER_POLL_PERIOD_MS 200
#define KEY_TIMER_POLL_TIMEOUT 5


static unsigned char exam_key_interrupt_on = 0;

static unsigned char exam_key_max_evt_size = 10;
static unsigned char exam_key_max_evt_num = 30;

static wiced_timer_t exam_key_timer;
static unsigned char exam_key_poll_enable=0;
static unsigned char exam_key_poll_timeout_cnt = KEY_TIMER_POLL_TIMEOUT;

void example_key_userKeyPressDetected(void* unused);
void example_key_poll(void);
void example_key_timerCb(uint32_t cBackparam);
void example_key_enable_poll(unsigned char enable);

scankey_cb_t scankey_cb = NULL;

//extern void handle_keycode_action(KeyEvent *key);


void example_key_Deinit(void)
{
    wiced_hal_mia_enable_mia_interrupt(FALSE);
    wiced_hal_mia_enable_lhl_interrupt(FALSE);
    wiced_hal_keyscan_turnOff();
}

void example_key_init(scankey_cb_t hanlde_scan_key)
{
    extern void wiced_hal_mia_notificationRegisterKeyscan(void);
    
    WICED_BT_TRACE("example_key_init \r\n");

    wiced_hal_keyscan_configure(NUM_KEYSCAN_ROWS, NUM_KEYSCAN_COLS);
    wiced_hal_keyscan_init();

    // reenforce GPIO configuration so we don't lose it after entering uBCS
    wiced_hal_gpio_slimboot_reenforce_cfg(WICED_P14, GPIO_OUTPUT_ENABLE);

    wiced_hal_keyscan_register_for_event_notification(example_key_userKeyPressDetected, NULL);

    wiced_hal_mia_notificationRegisterKeyscan();

    wiced_hal_mia_enable_mia_interrupt(TRUE);
    wiced_hal_mia_enable_lhl_interrupt(TRUE);
    scankey_cb = hanlde_scan_key;
    //keyscan_enableInterrupt();

    //wiced_init_timer(&exam_key_timer,example_key_timerCb,0,WICED_MILLI_SECONDS_TIMER);

    exam_key_poll_enable = 0;

    example_key_enable_poll(1);
}


// Keyscan interrupt
void example_key_userKeyPressDetected(void* unused)
{
    exam_key_interrupt_on = wiced_hal_keyscan_is_any_key_pressed();

    //WICED_BT_TRACE(" isr of key !\r\n");

    // Poll the app.
    example_key_poll();
}

void example_key_poll(void)
{
    /// Temporary used for creating key events
    KeyEvent my_key_evt;

    // Poll the hardware for events
    wiced_hal_mia_pollHardware();

    while (wiced_hal_keyscan_get_next_event(&my_key_evt))
    {
        {   INT8 row_t=-1;  //invalid scancode
            INT8 column_t=-1;

            if(my_key_evt.keyCode < END_OF_SCAN_CYCLE)
            {
                column_t = my_key_evt.keyCode / NUM_KEYSCAN_ROWS;
                row_t = my_key_evt.keyCode % NUM_KEYSCAN_ROWS;

                WICED_BT_TRACE(" keyCode=0x%x(column=%d,row=%d), %s \n",\
                    my_key_evt.keyCode, column_t,row_t,\
                    (my_key_evt.upDownFlag==KEY_DOWN)?"down":"up");
                //handle_keycode_action(&my_key_evt);
                if(scankey_cb != NULL)
                {
                    if(KEY_DOWN == my_key_evt.upDownFlag)
                    {
                        scankey_cb(my_key_evt.keyCode,HANDLE_KEY_DOWN);
                    }
                    else
                    {
                        scankey_cb(my_key_evt.keyCode,HANDLE_KEY_UP);
                    }
                    
                }

            }
            else if(my_key_evt.keyCode == ROLLOVER)
            {
                WICED_BT_TRACE(" Warning::ghost key!  keyCode = 0x%x\n",my_key_evt.keyCode);
                scankey_cb(my_key_evt.keyCode,HANDLE_KEY_UP);
            }
        }
    }


    if (wiced_hal_keyscan_is_any_key_pressed()==FALSE)
    {
        if(exam_key_poll_timeout_cnt)
            exam_key_poll_timeout_cnt--;
        else
            exam_key_poll_enable=0;
    }
    else
    {
        if(exam_key_poll_enable==0)
            example_key_enable_poll(1);
        else
            {
            if(exam_key_poll_timeout_cnt!=KEY_TIMER_POLL_TIMEOUT)
               exam_key_poll_timeout_cnt = KEY_TIMER_POLL_TIMEOUT;
        }
    }

}


void example_key_enable_poll(unsigned char enable)
{
    if(exam_key_poll_enable!=enable)
    {
        if(wiced_is_timer_in_use(&exam_key_timer)==TRUE)
            wiced_stop_timer(&exam_key_timer);

        if (enable)
        {
            wiced_start_timer(&exam_key_timer, KEY_TIMER_POLL_PERIOD_MS);
            WICED_BT_TRACE(" enable keyscan poll timer \r\n");
            exam_key_poll_timeout_cnt = KEY_TIMER_POLL_TIMEOUT;
        }
        else
        {
            WICED_BT_TRACE(" disable keyscan poll timer \r\n");
        }

        exam_key_poll_enable = enable;
    }
}

void example_key_timerCb(uint32_t cBackparam)
{
    //WICED_BT_TRACE("key poll timer cb,cnt=%d\r\n",exam_key_poll_timeout_cnt);

    if(wiced_is_timer_in_use(&exam_key_timer)==TRUE)
        wiced_stop_timer(&exam_key_timer);

    example_key_poll();

    if(exam_key_poll_enable)
    {
        wiced_start_timer(&exam_key_timer,KEY_TIMER_POLL_PERIOD_MS);
    }
}

void disable_key_scan_timer(void)
{
    if(wiced_is_timer_in_use(&exam_key_timer)==TRUE)
        wiced_stop_timer(&exam_key_timer);
}


