/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file template_api.h
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
/**
 *
 * @mainpage Welcome to the Documentation of the i2c driver
 *
 *
 * Doc-Id: xx-xxx-xxx-xxx (NAME Implementation Specification)\n
 * Author: Ulrich Marx
 *
 * DESCRIBE_HERE
 *
 *
 * The manual is divided into the following sections:
 *
 * -@subpage module_name_api_page \n
 * -@subpage module_name_page \n
 *
 * @page module_name_api_page Module Name API
 * This module is the API for the NAME. DESCRIBE IN DETAIL / ADD USECASES...
 *
 * for a detailed list of api functions refer to:
 * - @ref module_name_api
 *
 * @defgroup i2c_drv_api I2C Driver API
 * @{
 */
#ifndef __I2C_DRV_H__
#define __I2C_DRV_H__

#include <ebase/types.h>

/*******************************************************************************
 * @brief The i2c return value type.
 *
 */
typedef int32_t I2cReturnType;


/*******************************************************************************
 * @brief The i2c return values
 *
 * This enumeration lists the possible i2c return values.
 */
enum I2C_RESULT
{
    I2C_RET_SUCCESS             =   0,      /**< The operation was successfully completed */
    I2C_RET_FAILURE             =  -1,      /**< Generic error */
    I2C_RET_PENDING             =  -2,      /**< The operation is ongoing */
    I2C_RET_NOT_SUPPORTED       =  -3,      /**< The requested function is not supported */
    I2C_RET_TIMEOUT             =  -4,      /**< Something timed out */
    I2C_RET_BUSY                =  -5,      /**< The requested operation can't be fulfilled because the resource is busy */
    I2C_RET_INVALID_PARM        =  -6,      /**< First parameter is invalid */
    I2C_RET_CANCELLED           =  -7,      /**< The operation was cancelled */
    I2C_RET_NO_DATA             =  -8,      /**< Insufficient data for the operation to complete */
    I2C_RET_FULL                =  -9,      /**< Something is full */
    I2C_RET_OVERFLOW            = -10,      /**< Something was overflowing */
    I2C_RET_EMPTY               = -11,      /**< Something is empty */
    I2C_RET_UNDERFLOW           = -12,      /**< Something was underflowing */
    I2C_RET_NOT_FOUND           = -13,      /**< Unsuccessful find operation */
    I2C_RET_INVALID_HANDLE      = -14,      /**< The given handle is invalid */
    I2C_RET_INVALID_CONFIG      = -15,      /**< Configuration is not allowed */
    I2C_RET_NO_HARDWARE         = -16,      /**< Hardware not present */
    I2C_RET_WRONG_STATE         = -17,      /**< Module is in wrong state */
    I2C_RET_WRITE_ERROR         = -18,      /**< Write error */
    I2C_RET_READ_ERROR          = -19,      /**< Reads error */
    I2C_RET_PROTOCOLL_ERROR     = -20,      /**< i2c protocoll error */
    I2C_RET_INVALID_ADDRESS     = -21,      /**< Invalid address */
    I2C_RET_ONLY_ONE_INSTANCE   = -22,      /**< Only one instance is allowed */
    I2C_RET_NOT_AVAILABLE       = -23,      /**< Something is not available */
};


/**
 * @brief
 *
 * @note
 *
 */
typedef struct _i2c_bus
{
    uint32_t  ulControllerBaseAddress;      /**< Base Address of the I2C Controller */
    uint8_t   aucMasterAddrMode;            /**< Master Address Mode 1: 10 bit; 0: 7 bit */
    uint32_t  ulMasterAddr;                 /**< Master Device Address */
    uint32_t  ulSclRef;                     /**< Clock Divider Value */
    uint8_t   aucVirtualClockDividerEnable; /**< Virtual System Clock Divider 1: divide by 8; 0: no division */
    uint8_t   aucTimingMode;                /**< Timing Mode 1: fast; 0: standard */
    uint8_t   aucSpikeFilter;               /**< Suppressed Spike Width */
    uint8_t   aucIrqDisable;                /**< Interrupt Mask 1: interrupt disabled; 0: interrupt enabled */
} i2c_bus_t;



/**
 *          i2c_init()
 *
 * @brief   Initialize the ic2-bus-master module
 *
 * @note
 *
 */
extern I2cReturnType i2c_init
(
    const i2c_bus_t *i2c
);



/**
 *          i2c_read_reg8()
 *
 * @brief   Read an 8-Bit value from an 8-Bit addressable register
 *
 * @note
 *
 */
I2cReturnType i2c_read_reg8
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint8_t         reg_addr,
    uint8_t         *value
);



/**
 *          i2c_read_reg16()
 *
 * @brief   Read an 8-Bit value from a 16-Bit addressable register
 *
 * @note
 *
 */
I2cReturnType i2c_read_reg16
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint16_t        reg_addr,
    uint8_t         *value
);



/**
 *          i2c_read_ex()
 *
 * @brief   Read n bytes starting at location which is addressed via 0..3 bytes
 *
 * @note
 *
 */
extern I2cReturnType i2c_read_ex
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *data,
    uint8_t         num_data
);

extern I2cReturnType i2c_read_ex2
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data,
    int16_t         *pdata_read
);



/**
 *          i2c_write_reg8()
 *
 * @brief   Write an 8-Bit value to an 8-Bit addressable register
 *
 * @note
 *
 */
I2cReturnType i2c_write_reg8
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint8_t         reg_addr,
    uint8_t         value
);



/**
 *          i2c_write_reg8()
 *
 * @brief   Write an 8-Bit value to a 16-Bit addressable register
 *
 * @note
 *
 */
I2cReturnType i2c_write_reg16
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint16_t        reg_addr,
    uint8_t         value
);


/**
 *          i2c_write_ex()
 *
 * @brief   Write n bytes starting at location which is addressed via 0..3 bytes
 *
 * @note
 *
 */
extern I2cReturnType i2c_write_ex
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *data,
    uint8_t         num_data
);

extern I2cReturnType i2c_write_ex2
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data,
    int16_t         *pdata_written
);


#endif /* __I2C_DRV_H__ */
