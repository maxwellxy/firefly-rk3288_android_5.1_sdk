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
* @file hal_cosim.h
*
* <pre>
*
* Description:
*   This header file exports the register IO interface realized as a direct
*   memory access inline function. You should use it for your embedded
*   implementation.\n
*   Do not include directly! Include hal_api.h instead.
*
* </pre>
*/
/*****************************************************************************/

#ifndef __HAL_COSIM_H__
#define __HAL_COSIM_H__

//MEMSET, MEMCPY:
#include "ebase/builtins.h"
//bool, INLINE:
#include "ebase/types.h"

#include "ebase/dct_assert.h"
#include "ebase/trace.h"

#include <stdio.h>
//#include "i2c_drv/i2c_drv.h"

//#include "altera_fpga.h"

#if defined(HAL_COSIM)
#define  SIM_ERROR 2
#define  SIM_MESSAGE 3
#define  SIM_VERBINFO 4
#define  SIM_INFO 5
//extern void sim_fprintf(int slevel, char* info_string);
#endif

#undef TRACE
#undef TEST_ASSERT_EQUAL_INT
#undef TEST_ASSERT

#if defined(HAL_COSIM)

  extern void sim_fprintf(msg_severity_t slevel, char* info_string);

  #define TRACE(a,...) \
  {\
      char info_string[120];            \
    sprintf(info_string, __VA_ARGS__);      \
    sim_fprintf(SVE_INFO, info_string);     \
  }

  #define TEST_ASSERT_EQUAL_INT(a,b) \
  if ((a)!=(b)) {\
      char error_string[120];\
      sprintf(error_string, "expected=0x%08X, received=0x%08X",(a),(b) );  \
      sim_fprintf(SVE_ERROR, error_string);     \
  }

  #define TEST_ASSERT(...) \
  {\
    if (__VA_ARGS__) {} else {          \
      sim_fprintf(SVE_ERROR, "TEST_ASSERT");}   \
  }


#else
  #define TRACE(a,...) (printf(__VA_ARGS__))
  #define TEST_ASSERT_EQUAL_INT(a,b) if ((a)!=(b)) printf("ERROR: expected=0x%08X, received=0x%08X\n",(a),(b) )
  #define TEST_ASSERT(...) (printf("ERROR: TEST_ASSERT"))
#endif



//#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _i2c_bus {
    uint32_t  ulSclRef;                     //!< Clock Divider Value
    uint8_t   aucVirtualClockDividerEnable; //!< Virtual System Clock Divider 1: divide by 8; 0: no division
    uint8_t   aucTimingMode;                //!< Timing Mode 1: fast; 0: standard
    uint8_t   aucSpikeFilter;               //!< Suppressed Spike Width
    uint8_t   aucIrqDisable;                //!< Interrupt Mask 1: interrupt disabled; 0: interrupt enabled
} i2c_bus_t;




  //#include <embUnit/embUnit.h>

#define MRV_ALL_BASE (0x0000)
#define VDU_ALL_BASE (0x8000)
#define MRV2_ALL_BASE (0x10000)

#define IIC1_BASE (0x800)
#define IIC2_BASE (0x1000)
#define IIC3_BASE (0x1800)


#include <mrv_all_regs_macro_defs.h>


#include <mrv_all_regs_addr_map.h>
#include <mrv2_all_regs_addr_map.h>
#include <mrv_all_reg_description.h>
#include <mrv_all_reg_descr_idx.h>
  //#include <mrv_all_regs_mask.h>
#include <mrv_all_regs.h>

#include <vdu_all_regs_addr_map.h>
#include <vdu_all_reg_description.h>
#include <vdu_all_reg_descr_idx.h>
#include <vdu_all_regs_mask.h>

#include <fpga_sys_ctrl_regs_addr_map.h>
#include <fpga_sys_ctrl_reg_description.h>
#include <fpga_sys_ctrl_reg_descr_idx.h>
#include <fpga_sys_ctrl_regs_mask.h>

#include <iic1_regs_addr_map.h>
#include <iic2_regs_addr_map.h>
#include <iic3_regs_addr_map.h>
#include <iic1_reg_description.h>
#include <iic1_reg_descr_idx.h>
#include <iic1_regs_mask.h>


#define MARVIN_LITE 1
#define ISP_LITE 1

  //#define MVDU_BASE       0x8000
  //#define MVDU_REG_ID     0x0004
  //#define MVDU_REG_MSK    0x0080
  //#define MVDU_REG_RIS    0x0084
  //#define MVDU_REG_MIS    0x0088
  //#define MVDU_REG_ICR    0x008C
  //#define MVDU_REG_ISR    0x0090

extern sc_testcase* testcase_ptr;

#define writeReg testcase_ptr->writeReg
#define readReg testcase_ptr->readReg
#define writeSysReg testcase_ptr->writeSysReg
#define readSysReg testcase_ptr->readSysReg
#define wait_clk testcase_ptr->wait_clk


#define write_memory testcase_ptr->write_memory
#define read_memory testcase_ptr->read_memory

  //  HalHandle_t dummyHandle;


  //HalContext_s halContext;
  //HalHandle_t dummyHandle;
#define HAL_DEVID_PCLK      0x00000008  //!< VDU.pclk

/******************************************************************************
 * HalReadReg()
 *****************************************************************************/
INLINE uint32_t HalReadReg( HalHandle_t handle, ulong_t reg_address )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);
    //    return *( (volatile uint32_t *) reg_address );

    return (readReg(handle, reg_address));
}


/******************************************************************************
 * HalWriteSysReg()
 *****************************************************************************/
INLINE void HalWriteSysReg( HalHandle_t handle, ulong_t reg_address, uint32_t value )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);
    //    *( (volatile uint32_t*) reg_address ) = value;

    writeSysReg(handle, reg_address, value);
}


/******************************************************************************
 * HalReadSysReg()
 *****************************************************************************/
INLINE uint32_t HalReadSysReg( HalHandle_t handle, ulong_t reg_address )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);
    //    return *( (volatile uint32_t *) reg_address );

    return (readSysReg(handle, reg_address));
}


/******************************************************************************
 * HalWriteReg()
 *****************************************************************************/
INLINE void HalWriteReg( HalHandle_t handle, ulong_t reg_address, uint32_t value )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);
    //    *( (volatile uint32_t*) reg_address ) = value;

    writeReg(handle, reg_address, value);
}


/******************************************************************************
 * HalReadMaskedReg()
 *****************************************************************************/
INLINE uint32_t HalReadMaskedReg( HalHandle_t handle, ulong_t reg_address, uint32_t reg_mask, uint32_t shift_mask )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);

    uint32_t tmp_value = HalReadReg( handle, reg_address );
    return HalGetMaskedValue( tmp_value, reg_mask, shift_mask );
}


/******************************************************************************
 * HalWriteMaskedReg()
 *****************************************************************************/
INLINE void HalWriteMaskedReg( HalHandle_t handle, ulong_t reg_address, uint32_t reg_mask, uint32_t shift_mask, uint32_t value )
{
    (void) handle;
    DCT_ASSERT(handle != NULL);

    uint32_t tmp_value = HalReadReg( handle, reg_address );
    tmp_value = HalSetMaskedValue( tmp_value, reg_mask, shift_mask, value );
    HalWriteReg( handle, reg_address, tmp_value );
}


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
#define NUM_I2C 3

#define SYSCTRL_REVID_OFFS  0x00
#define SYSCTRL_SELECT_OFFS 0x10
#define SYSCTRL_RESET_OFFS  0x20

#define SYSCTRL_SELECT_CAM_1_NEGEDGE 0x00000100
#define SYSCTRL_SELECT_CAM_1_RESET   0x00000200
#define SYSCTRL_SELECT_CAM_1_POWERDN 0x00000400

#define EXT_MEM_ALIGN    0x1000 // 4k
#define EXT_MEM_BASE     EXT_MEM_ALIGN // musn't be 0!; will be aligned up to EXT_MEM_ALIGNment
#define EXT_MEM_SIZE (0x08000000 - EXT_MEM_BASE) // ~128MB

#define DMA_STRIDE_BYTES 8 ////FPGA_DMA_SIZE_ALIGNMENT // currently 32

#define HAL_USE_RAW_DMA // comment out to use dma functions with byte reordering & stuff


/******************************************************************************
 * local type definitions
 *****************************************************************************/
typedef struct HalCamConfig_s
{
    bool_t configured;      //!< Mark whether this config was set.
    bool_t power_lowact;    //!< Power on is low-active.
    bool_t reset_lowact;    //!< Reset is low-active.
} HalCamConfig_t;


/******************************************************************************
 * HalContext_t
 *****************************************************************************/
typedef struct HalContext_s
{
  osMutex         modMutex;               //!< common short term mutex; e.g. for read-modify-write accesses
  uint32_t        refCount;               //!< internal ref count

  osMutex         iicMutex[NUM_I2C];      //!< transaction locks for I2C controller 1..NUM_I2C
  i2c_bus_t       iicConfig[NUM_I2C];     //!< configurations for I2C controller 1..NUM_I2C

  HalCamConfig_t  cam1Config;             //!< configuration for CAM1; set at runtime
} HalContext_t;

HalContext_s halContext;

/******************************************************************************
 * local variable declarations
 *****************************************************************************/
//static HalContext_t gHalContext  = { .refCount = 0 };
//static bool_t       gInitialized = BOOL_FALSE;

/******************************************************************************
 * local function prototypes
 *****************************************************************************/
//static int32_t halIsrHandler( void *pArg );


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * HalOpen()
 *****************************************************************************/
HalHandle_t HalOpen( void )
{

  HalHandle_t localDummyHandle;

  localDummyHandle = &halContext;
  return localDummyHandle;
}


/******************************************************************************
 * HalClose()
 *****************************************************************************/
RESULT HalClose( HalHandle_t HalHandle )
{
    RESULT result = RET_SUCCESS;
    return result;
}


/******************************************************************************
 * HalAddRef()
 *****************************************************************************/
RESULT HalAddRef( HalHandle_t HalHandle )
{
    RESULT result = RET_SUCCESS;
    return result;
}


/******************************************************************************
 * HalDelRef()
 *****************************************************************************/
RESULT HalDelRef( HalHandle_t HalHandle )
{
    // just forward to HalClose; it takes care of everything
    return HalClose( HalHandle );
}


/******************************************************************************
 * HalSetCamConfig()
 *****************************************************************************/
RESULT HalSetCamConfig( HalHandle_t HalHandle, uint32_t dev_mask, bool_t power_lowact, bool_t reset_lowact, bool_t pclk_negedge )
{
    RESULT result = RET_NOTSUPP;
    return result;
}

/******************************************************************************
 * HalSetReset()
 *****************************************************************************/
RESULT HalSetReset( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_NOTSUPP;
    return result;
}


/******************************************************************************
 * HalSetPower()
 *****************************************************************************/
RESULT HalSetPower( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_NOTSUPP;
    return result;
}


/******************************************************************************
 * HalSetClock()
 *****************************************************************************/
static uint32_t _set_single_pll_counter(HalHandle_t HalHandle, const uint8_t counter_type, const uint16_t value);
static uint32_t _set_single_pll_param(HalHandle_t HalHandle, const uint8_t counter_type, const uint8_t counter_param, const uint16_t value);
RESULT HalSetClock( HalHandle_t HalHandle, uint32_t dev_mask, uint32_t frequency )
{
  uint32_t reset_val;
  uint32_t res;
  RESULT result;

  uint32_t i;


  result = RET_SUCCESS;

  // there are problems with reconfiguring the pll during simulation. Thats why this feature is switched off.
  // use correct pll init settings during compile
#if 0

  if (dev_mask == HAL_DEVID_PCLK) {
    reset_val = HalReadSysReg(HalHandle, MRV_FPGA_VEC_SYS_RESETN_REG);
    HalWriteSysReg(HalHandle, MRV_FPGA_VEC_SYS_RESETN_REG, reset_val & 0xff7f);
    HalWriteSysReg(HalHandle, MRV_FPGA_VEC_SYS_RESETN_REG, reset_val | 0x0080);


    switch (frequency)
      {
      case  25200000ul:
        res = _set_single_pll_counter(HalHandle, 0, 5);
        res = _set_single_pll_counter(HalHandle, 1, 105);

        res = _set_single_pll_counter(HalHandle, 4, 45);
        res = _set_single_pll_counter(HalHandle, 5, 14);
        break;
      case  74250000ul:
        res = _set_single_pll_counter(HalHandle, 0, 1); //2);  //N
        res = _set_single_pll_counter(HalHandle, 1, 33); //33); //M

        res = _set_single_pll_counter(HalHandle, 4, 24); //C0
        res = _set_single_pll_counter(HalHandle, 5, 22); //C1
        res = _set_single_pll_counter(HalHandle, 6, 12);
        res = _set_single_pll_counter(HalHandle, 7, 12);
        res = _set_single_pll_counter(HalHandle, 8, 12);
        res = _set_single_pll_counter(HalHandle, 9, 12);
        res = _set_single_pll_counter(HalHandle, 10, 12);
        res = _set_single_pll_counter(HalHandle, 11, 12);
        res = _set_single_pll_counter(HalHandle, 12, 12);
        res = _set_single_pll_counter(HalHandle, 13, 12);
        break;
      case 148500000ul:
        res = _set_single_pll_counter(HalHandle, 0, 2);
        res = _set_single_pll_counter(HalHandle, 1, 33);

        res = _set_single_pll_counter(HalHandle, 4, 6);
        res = _set_single_pll_counter(HalHandle, 5, 11);
        break;
      default:
        return RET_NOTSUPP;
      }

    for (i=0; i<50;i++) {
      reset_val = HalReadSysReg(HalHandle, MRV_FPGA_VEC_SYS_RESETN_REG);
    }

    HalWriteSysReg(HalHandle, MRV_FPGA_RE_RECONFIG_REG, 0x1);

  }
  else {
    result = RET_NOTSUPP;
  }

#endif

    return result;
}


/******************************************************************************
 * HalAllocMemory()
 *****************************************************************************/
ulong_t HalAllocMemory( HalHandle_t HalHandle, uint32_t byte_size )
{
  //    if (HalHandle == NULL)
  //    {
  //        return RET_NULL_POINTER;
  //    }
  return  ((ulong_t) malloc(byte_size));
}


/******************************************************************************
 * HalFreeMemory()
 *****************************************************************************/
RESULT HalFreeMemory( HalHandle_t HalHandle, ulong_t mem_address )
{
  //    if (HalHandle == NULL)
  //    {
  //        return RET_NULL_POINTER;
  //    }

  free((void *)mem_address);
    mem_address = NULL;

    return RET_SUCCESS;
}


/******************************************************************************
 * HalReadMemory()
 *****************************************************************************/
RESULT HalReadMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_read_buffer, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    //DCT_ASSERT((mem_address & (DMA_STRIDE_BYTES-1)) == 0);
    DCT_ASSERT((byte_size & (DMA_STRIDE_BYTES-1)) == 0);
    read_memory(mem_address, p_read_buffer, byte_size); // == 0) ? RET_SUCCESS : RET_FAILURE;

    return (RET_SUCCESS);
}

/******************************************************************************
 * HalWriteMemory()
 *****************************************************************************/
RESULT HalWriteMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_write_buffer, uint32_t byte_size )
{
  //if (HalHandle == NULL)
  //    {
  //        return RET_NULL_POINTER;
  //    }

    //DCT_ASSERT((mem_address & (DMA_STRIDE_BYTES-1)) == 0);
    DCT_ASSERT((byte_size & (DMA_STRIDE_BYTES-1)) == 0);

    write_memory(  mem_address, p_write_buffer,byte_size ); // == 0) ? RET_SUCCESS : RET_FAILURE;
    return ( RET_SUCCESS );
}


/******************************************************************************
 * HalReadI2CMem()
 *****************************************************************************/
RESULT HalReadI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    ulong_t    reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_read_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_NOTSUPP;
    return result;
}


/******************************************************************************
 * HalWriteI2CMem()
 *****************************************************************************/
RESULT HalWriteI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    ulong_t    reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_write_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_NOTSUPP;
    return result;
}



/******************************************************************************
 * HalConnectIrq()
 *****************************************************************************/
RESULT HalConnectIrq
(
    HalHandle_t HalHandle,
    HalIrqCtx_t *pIrqCtx,
    uint32_t    int_src,
    osIsrFunc   IsrFunction,
    osDpcFunc   DpcFunction,
    void*       pContext
)
{
    RESULT result = RET_NOTSUPP;
    return result;
}


/******************************************************************************
 * HalDisconnectIrq()
 *****************************************************************************/
RESULT HalDisconnectIrq
(
    HalIrqCtx_t *pIrqCtx
)
{
    RESULT result = RET_NOTSUPP;
    return result;
}

/******************************************************************************
 * Local functions
 *****************************************************************************/
/*******************************************************************************/

static uint32_t _set_single_pll_counter(HalHandle_t HalHandle, const uint8_t counter_type, const uint16_t value) {
  uint32_t res;

  res = _set_single_pll_param(HalHandle, counter_type, 4, value ? 0 : 1);  /* bypass */
  res = _set_single_pll_param(HalHandle, counter_type, 0, (value + 1) >> 1);  /* high count */
  res = _set_single_pll_param(HalHandle, counter_type, 5, value & 1);  /* odd/even */
  res = _set_single_pll_param(HalHandle, counter_type, 1, value >> 1);  /* low count */

  return RET_SUCCESS;
}
/*******************************************************************************/

static uint32_t _set_single_pll_param(HalHandle_t HalHandle, const uint8_t counter_type, const uint8_t counter_param, const uint16_t value) {

  HalWriteSysReg(HalHandle, MRV_FPGA_RE_COUNTER_TYPE_PARAM_REG, (counter_param << 4) | counter_type );
  HalWriteSysReg(HalHandle, MRV_FPGA_RE_WR_RD_DATA_REG, value);
  HalWriteSysReg(HalHandle, MRV_FPGA_RE_WRITE_PARAM_REG, 0x1);

  return RET_SUCCESS;
}

/*******************************************************************************/


/******************************************************************************
 * halIsrHandler
 *****************************************************************************/
//static int32_t halIsrHandler( void *pArg )
//{
//    return ( 0 );
//}





#ifdef __cplusplus
}
#endif

#endif /* __HAL_COSIM_H__ */


