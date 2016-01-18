/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: log_printf_linux.c 5629 2011-06-11 03:13:08Z mcaramello $ 
 *
 ******************************************************************************/
 
/**
 * @addtogroup MPL_LOG
 *
 * @{
 *      @file   log_printf.c
 *      @brief  printf replacement for _MLWriteLog.
 */

#include <stdio.h>
#include "log.h"

int _MLWriteLog (const char * buf, int buflen)
{
    return fputs(buf, stdout);
}

/**
 * @}
 */

