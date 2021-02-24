#if _ANDONDEBUG
#include <string.h>
#include <stdio.h>
#include "log.h"

#include "wiced_bt_gatt.h"

#define HANDLE_ANDON_DATA_OUT_VALUE 0x62

extern uint16_t conn_id;
static char header[128];
static char log_buffer[128];
static char total_buffer[256];

char *file_name_of(const char *file)
{
    const int total_len = strlen(file);
    char *p = (char *)file + total_len;

    while ((p > file) && (*p != '/') && (*p != '\\'))
    {
        p--;
    }

    if ((*p == '/') || (*p == '\\'))
        p++;

    return p;
}

void my_sprintf(char *str, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
}

void write_multi_routine(const char *format, const char *file, int line, ...)
{
    va_list args;
    my_sprintf(header, "%s, %d: ", file_name_of(file), line);
    va_start(args, line);
    vsprintf(log_buffer, format, args);
    va_end(args);

    my_sprintf(total_buffer, "%s %s", header, log_buffer);

    if (conn_id != 0)
    {
        //TODO: send to gatt client
        // wiced_bt_gatt_send_notification(conn_id, HANDLE_ANDON_DATA_OUT_VALUE, strlen(total_buffer), total_buffer);
    }

    // WICED_BT_TRACE(total_buffer);
    wiced_printf(NULL,0,total_buffer);
}
#endif