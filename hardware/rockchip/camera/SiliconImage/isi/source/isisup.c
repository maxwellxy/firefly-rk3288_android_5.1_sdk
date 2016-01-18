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
 * @file isisup.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( ISI_INFO  );
USE_TRACER( ISI_WARN  );
USE_TRACER( ISI_ERROR );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 *          IsiWriteRegister
 *
 * @brief   writes a given number of bytes to the image sensor device by
 *          calling the corresponding sensor-function
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   RegValue            value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiWriteRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress, 
    const uint32_t      RegValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pSensorCtx == NULL) || (pSensorCtx->pSensor == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pSensorCtx->pSensor->pIsiRegisterWriteIss == NULL )
    {
        return ( RET_NOTSUPP );
    }

    result = pSensorCtx->pSensor->pIsiRegisterWriteIss( handle, RegAddress, RegValue );

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          IsiReadRegister
 *
 * @brief   reads a given number of bytes from the image sensor device 
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   RegValue            value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiReadRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress, 
    uint32_t            *pRegValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pSensorCtx == NULL) || (pSensorCtx->pSensor == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRegValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( pSensorCtx->pSensor->pIsiRegisterReadIss == NULL )
    {
        return ( RET_NOTSUPP );
    }

    result = pSensorCtx->pSensor->pIsiRegisterReadIss( handle, RegAddress, pRegValue );

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          IsiI2cSwapBytes
 *
 * @brief   This function swaps the byte-order.
 *
 * @param   pData               pointer to data memory
 * @param   NrOfDataBytes       number of bytes in data memory
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_NULL_POINTER
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT IsiI2cSwapBytes
(
    uint8_t         *pData, 
    const uint8_t   NrOfDataBytes 
)
{
    RESULT result = RET_FAILURE;

    TRACE( ISI_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pData == NULL  )
    {
        return ( RET_NULL_POINTER );
    }

    switch ( NrOfDataBytes )
    {
        case 1:
            {
                // nothing to do
                result = RET_SUCCESS;
                break;
            }

        case 2:
            {
                uint8_t *pSwapData = pData;
                uint8_t  SwapByte = 0;

                // advance to second byte
                pSwapData++;

                // save first byte
                SwapByte = *(pData);

                // copy second byte to first position
                *pData = *pSwapData;

                // restore first byte to second position
                *pSwapData = SwapByte;

                result = RET_SUCCESS;
                break;
            }

        case 4:
            {
                uint32_t *pSwapData = (uint32_t *)(pData);
                uint32_t  Help = 0UL;

                // get byte 1
                Help = (*pSwapData & 0x000000FF);
                Help <<= 8;
                *pSwapData >>= 8;

                // subjoin byte 2
                Help |= (*pSwapData & 0x000000FF);
                Help <<= 8;
                *pSwapData >>= 8;

                // subjoin byte 3
                Help |= (*pSwapData & 0x000000FF);
                Help <<= 8;
                *pSwapData >>= 8;

                // get byte 4
                *pSwapData  &= 0x000000FF;

                // subjoin bytes 1 to 3
                *pSwapData  |= Help;

                result = RET_SUCCESS;
                break;
            }

        default:
            {
                TRACE( ISI_ERROR, "%s: Wrong amount of bytes (%d) for swapping.\n", __FUNCTION__, NrOfDataBytes );
                break;
            }
    }

    TRACE( ISI_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/*****************************************************************************/
/**
 *          IsiI2cWriteSensorRegister
 *
 * @brief   writes a given number of bytes to the image sensor device via i2c
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   data                pointer to data-bytes
 * @param   NrOfDataBytes       number of data-bytes to write
 * @param   bSwapBytesEnable    swap bytes
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiI2cWriteSensorRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress, 
    uint8_t             *pData, 
    const uint8_t       NrOfDataBytes, 
    const bool_t        bSwapBytesEnable
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pData == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( bSwapBytesEnable == BOOL_TRUE )
    {
        /* swap bytes */
        IsiI2cSwapBytes ( pData, NrOfDataBytes );
    }

    result = HalWriteI2CMem( pSensorCtx->HalHandle, 
                                pSensorCtx->I2cBusNum, 
                                pSensorCtx->SlaveAddress, 
                                RegAddress, 
                                pSensorCtx->NrOfAddressBytes, 
                                pData, 
                                NrOfDataBytes );

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          IsiI2cReadSensorRegister
 *
 * @brief   reads a given number of bytes from the image sensor device 
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   data                pointer to data-bytes
 * @param   NrOfDataBytes       number of data-bytes to write
 * @param   bSwapBytesEnable    swap bytes
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiI2cReadSensorRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress,
    uint8_t             *pData,
    const uint8_t       NrOfDataBytes,
    const bool_t        bSwapBytesEnable
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pData == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    result = HalReadI2CMem( pSensorCtx->HalHandle, 
                                pSensorCtx->I2cBusNum, 
                                pSensorCtx->SlaveAddress, 
                                RegAddress, 
                                pSensorCtx->NrOfAddressBytes, 
                                pData, 
                                NrOfDataBytes );

    if ( bSwapBytesEnable == BOOL_TRUE )
    {
        /* swap bytes */
        IsiI2cSwapBytes ( pData, NrOfDataBytes );
    }

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          IsiGetNrDatBytesIss
 *
 * @brief   Extracts from the register description table the amount of bytes 
 *          the desired register consists of
 *
 * @param   address          register address
 * @param   pRegDesc         register description table 
 *
 * @return  Return the number of bytes 
 * @retval  ISI_I2C_NR_DAT_BYTES_1
 * @retval  ISI_I2C_NR_DAT_BYTES_2
 * @retval  ISI_I2C_NR_DAT_BYTES_4
 * @retval  0U                      unknown register
 *
 *****************************************************************************/
uint8_t IsiGetNrDatBytesIss
(
    const uint32_t              address,
    const IsiRegDescription_t   *pRegDesc
)
{
    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pRegDesc != NULL )
    {
        IsiRegDescription_t *pCurrPtr = (IsiRegDescription_t *)pRegDesc;
        while ( ( pCurrPtr->Addr != address ) && ( pCurrPtr->Flags != eTableEnd ) )
        {
            pCurrPtr++;
        }

        if( pCurrPtr->Flags != eTableEnd )
        {
            switch ( (pCurrPtr->Flags & ((uint32_t)(eTwoBytes | eFourBytes))) )
            {
                case 0:
                    {
                        return ( ISI_I2C_NR_DAT_BYTES_1 );
                    }
                case eTwoBytes:
                    {
                        return ( ISI_I2C_NR_DAT_BYTES_2 );
                    }
                case eFourBytes:
                    {
                        return ( ISI_I2C_NR_DAT_BYTES_4 );
                    }

                default:
                    {
                        // nothing to do, as 0 will be returned
                        break;
                    }
            }
        }
    }

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0U );
}



/*****************************************************************************/
/**
 *          IsiRegDefaultsApply
 *
 * @brief   This function applies the default values of the registers specified
 *          in the given table. Writes to all registers that have been declared 
 *          as writable and do have a default value (appropriate enum in table).
 *
 * @param   handle      Handle to image sensor device
 * @param   pRegDesc    Register description table 
 *
 * @return  Return the number of bytes 
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiRegDefaultsApply
(
    IsiSensorHandle_t         handle,
    const IsiRegDescription_t *pRegDesc
)
{
    RESULT result = RET_SUCCESS;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pRegDesc != NULL );

    while ( pRegDesc->Flags != eTableEnd)
    {
        /* if the register is writeable and has a default value */
        if ( (pRegDesc->Flags & eWritable) && !(pRegDesc->Flags & eNoDefault) )
        {
            result = IsiWriteRegister( handle, pRegDesc->Addr, pRegDesc->DefaultValue );
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }
        }

//#ifdef __DELAY_HACK_WITH_DEFAULT_VALUE__
        /* some registers need some delay after reading or writing */
        if ( pRegDesc->Flags & eDelay )
        {
            //wait user defined ms
            osSleep( pRegDesc->DefaultValue );
        }
//#endif        

        ++pRegDesc;
    }

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          IsiRegDefaultsVerify
 *
 * @brief   Reads back registers from the image sensor and compares them 
 *          against the register table. Only registers that are readable, 
 *          writable and not volatile will be checked. 
 *
 * @param   handle      Handle to image sensor device
 * @param   pRegDesc    Register description table 
 *
 * @return  Return the number of bytes 
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS 
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiRegDefaultsVerify
(
    IsiSensorHandle_t         handle,
    const IsiRegDescription_t *pRegDesc
)
{
    RESULT result = RET_SUCCESS;
    uint32_t value;

    TRACE( ISI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pRegDesc != NULL );

    while ( pRegDesc->Flags != eTableEnd )
    {
        /* if the register is readable and writable, but neither volatile nor without default value... */
        if( (pRegDesc->Flags & eReadWriteVolNoDef) == eReadWrite )
        {
            /* read register from image sensor */
            result = IsiReadRegister( handle, pRegDesc->Addr, &value );
            if ( result != RET_SUCCESS )
            {
                TRACE( ISI_ERROR, "%s: failed to read %36s @ 0x%04x, code %d\n", 
                            __FUNCTION__,  pRegDesc->pName, pRegDesc->Addr, result );
                return ( result );
            }

            /* does it match what we wrote before? */
            if ( value != pRegDesc->DefaultValue )
            {
             //   TRACE( ISI_WARN, "%s: %36s @ 0x%04x is 0x%08x instead of 0x%08x\n",
            //                __FUNCTION__, pRegDesc->pName, pRegDesc->Addr, value, pRegDesc->DefaultValue );
                TRACE( ISI_WARN, "%s:  @ 0x%04x is 0x%08x instead of 0x%08x\n",
                            __FUNCTION__,  pRegDesc->Addr, value, pRegDesc->DefaultValue );
            //zyc,for test
            
           //     return ( RET_FAILURE );
            }
            else
            {
                TRACE( ISI_INFO, "%s: %36s @ 0x%04x is 0x%08x -- ok\n",
                            __FUNCTION__, pRegDesc->pName, pRegDesc->Addr, value );
            }
        }
 
        ++pRegDesc;
    }

    TRACE( ISI_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}


