/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 * $Id: log_linux.c 5629 2011-06-11 03:13:08Z mcaramello $ 
 ******************************************************************************/
 
/**
 * @defgroup MPL_LOG
 * @brief Logging facility for the MPL
 *
 * @{
 *      @file     log.c
 *      @brief    Core logging facility functions.
 *
 *
**/

#include <stdio.h>
#include <string.h>
#include "log.h"
#include "mltypes.h"

#define LOG_BUFFER_SIZE (256)

#ifdef WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

int _MLPrintLog (int priority, const char* tag, const char* fmt, ...)
{
    va_list ap;
    int result;

    va_start(ap, fmt);
    result = _MLPrintVaLog(priority,tag,fmt,ap);
    va_end(ap);

    return result;
}

int _MLPrintVaLog(int priority, const char* tag, const char* fmt, va_list args)
{
    int result;
    char buf[LOG_BUFFER_SIZE];
    char new_fmt[LOG_BUFFER_SIZE];
    char priority_char;

    if (NULL == fmt) {
        fmt = "";
    }

    switch (priority) {
    case MPL_LOG_UNKNOWN:
        priority_char = 'U';
        break;
    case MPL_LOG_VERBOSE:
        priority_char = 'V';
        break;
    case MPL_LOG_DEBUG:
        priority_char = 'D';
        break;
    case MPL_LOG_INFO:
        priority_char = 'I';
        break;
    case MPL_LOG_WARN:
        priority_char = 'W';
        break;
    case MPL_LOG_ERROR:
        priority_char = 'E';
        break;
    case MPL_LOG_SILENT:
        priority_char = 'S';
        break;
    case MPL_LOG_DEFAULT:
    default:
        priority_char = 'D';
        break;
    };

    result = snprintf(new_fmt, sizeof(new_fmt), "%c/%s:%s", 
                       priority_char, tag, fmt);
    if (result <= 0) {
        return INV_ERROR_LOG_MEMORY_ERROR;
    }
    result = vsnprintf(buf,sizeof(buf),new_fmt, args);
    if (result <= 0) {
        return INV_ERROR_LOG_OUTPUT_ERROR;
    }
    
    result = _MLWriteLog(buf, strlen(buf));
    return INV_SUCCESS;
}

/**
 * @}
**/


