/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: ustore_mlsl_io.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/


#include <string.h>

#include "ustore_manager.h"
#include "ustore_delegate_io.h"
#include "ustore_manager_io.h"

#include "mlsl.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ustore-mlsl-io"

#define USTORE_STATE_CLOSED      0
#define USTORE_STATE_STORE_OPEN  1
#define USTORE_STATE_LOAD_OPEN   2

#define STORE_CAL_SIZE 2800

/* Don't allow overlapping store and load. 
 * (Just in case of a programming error in
 *  manager or delegate.)
 */
static int state = USTORE_STATE_CLOSED;

/* Buffer used for store and load */
static unsigned char cal_data[STORE_CAL_SIZE];
static unsigned char *cal_end = &cal_data[STORE_CAL_SIZE-1];

/* ustore state */
static unsigned char * pcal;
static unsigned char * section_end;

inv_error_t inv_ustoreload_set_max_len(int l)
{
    if (state == USTORE_STATE_CLOSED)
        return INV_ERROR;

    if (pcal + l <= cal_end){
        section_end = pcal + l;
        return INV_SUCCESS;
    } else {
        return INV_ERROR_MEMORY_EXAUSTED;
    }

}

void inv_ustoreload_reset_len(void)
{
    section_end = NULL;
}

/* ------------- S T O R E - R O U T I N E S ------- */

inv_error_t inv_ustore_open(void)
{
    if (state != USTORE_STATE_CLOSED)
        return INV_ERROR;
    
    pcal = cal_data;
    section_end = NULL;
    cal_end = cal_data + STORE_CAL_SIZE;
    state = USTORE_STATE_STORE_OPEN;
    return INV_SUCCESS;
}

inv_error_t inv_ustore_close(void)
{
    inv_error_t result;
    int store_length;
    if (state != USTORE_STATE_STORE_OPEN)
        return INV_ERROR;

    store_length = (pcal - cal_data) * sizeof(unsigned char);
    result = inv_serial_write_cal(cal_data, store_length);
    if (result != INV_SUCCESS)
        LOG_RESULT_LOCATION(result);

    section_end = NULL;
    cal_end = NULL;
    state = USTORE_STATE_CLOSED;
    return result;
}

inv_error_t inv_ustore_byte(unsigned char b)
{
    if (state != USTORE_STATE_STORE_OPEN)
        return INV_ERROR;
    if (section_end != NULL && 
        (pcal + sizeof(unsigned char)) >= section_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    if ((pcal + sizeof(unsigned char)) >= cal_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    
    *pcal = b;
    pcal += sizeof(unsigned char);
    return INV_SUCCESS;
}

inv_error_t inv_ustore_mem(const void *b, int length)
{
    if (state != USTORE_STATE_STORE_OPEN)
        return INV_ERROR;
    if (section_end != NULL && 
        (pcal + length) > section_end) // FIX FOR ALL?? (pcal + length) >= section_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    if (pcal + length >= cal_end)
        return INV_ERROR_MEMORY_EXAUSTED;
 
    memcpy(pcal, b, length);
    pcal += length;
    return INV_SUCCESS;
}

/* ------------- L O A D - R O U T I N E S ------- */

inv_error_t inv_uload_open(void)
{
    inv_error_t result;
    unsigned int store_length;
    if (state != USTORE_STATE_CLOSED)
        return INV_ERROR;

    result = inv_serial_read_cal((unsigned char*) &store_length, sizeof(store_length));
    if (result != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    if (store_length > STORE_CAL_SIZE)
        return INV_ERROR_MEMORY_EXAUSTED;

    result = inv_serial_read_cal(cal_data, store_length); 
    if (result != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    pcal = cal_data + sizeof(store_length);
    section_end = NULL;
    state = USTORE_STATE_LOAD_OPEN;
    return INV_SUCCESS;
}

inv_error_t inv_uload_close(void)
{
    if (state != USTORE_STATE_LOAD_OPEN)
        return INV_ERROR;

    section_end = NULL;
    cal_end = NULL;
    state = USTORE_STATE_CLOSED;
    return INV_SUCCESS;
}


inv_error_t inv_uload_byte(unsigned char *b)
{
    if (state != USTORE_STATE_LOAD_OPEN)
        return INV_ERROR;
    if (section_end != NULL && 
        (pcal + sizeof(unsigned char)) >= section_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    if ((pcal + sizeof(unsigned char)) >= cal_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    
    *b = *pcal;
    pcal++;
    return INV_SUCCESS;
}

inv_error_t inv_uload_mem(void *b, int length)
{
    if (state != USTORE_STATE_LOAD_OPEN)
        return INV_ERROR;
    if (section_end != NULL &&
		(pcal + length) > section_end)
        return INV_ERROR_MEMORY_EXAUSTED;
    if (pcal + length >= cal_end)
        return INV_ERROR_MEMORY_EXAUSTED;

    memcpy(b, pcal, length);
    pcal += length;
    return INV_SUCCESS;
}



