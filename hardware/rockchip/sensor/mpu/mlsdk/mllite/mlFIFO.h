/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

#ifndef INVENSENSE_INV_FIFO_H__
#define INVENSENSE_INV_FIFO_H__

#include "mltypes.h"
#include "mlinclude.h"
#include "ml.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlFIFO_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**************************************************************************/
    /*  Elements                                                              */
    /**************************************************************************/

#define INV_ELEMENT_1                    (0x0001)
#define INV_ELEMENT_2                    (0x0002)
#define INV_ELEMENT_3                    (0x0004)
#define INV_ELEMENT_4                    (0x0008)
#define INV_ELEMENT_5                    (0x0010)
#define INV_ELEMENT_6                    (0x0020)
#define INV_ELEMENT_7                    (0x0040)
#define INV_ELEMENT_8                    (0x0080)

#define INV_ALL                          (0xFFFF)
#define INV_ELEMENT_MASK                 (0x00FF)

    /**************************************************************************/
    /*  Accuracy                                                              */
    /**************************************************************************/

#define INV_16_BIT                       (0x0100)
#define INV_32_BIT                       (0x0200)
#define INV_ACCURACY_MASK                (0x0300)

    /**************************************************************************/
    /*  Accuracy                                                              */
    /**************************************************************************/

#define INV_GYRO_FROM_RAW                (0x00)
#define INV_GYRO_FROM_QUATERNION         (0x01)

    /**************************************************************************/
    /*  High Rate Proceses                                                    */
    /**************************************************************************/

#define MAX_HIGH_RATE_PROCESSES 16

    /**************************************************************************/
    /*  The value of inv_get_gyro_sum_of_sqr is scaled such the (1 dps)^2 =   */
    /*  2^GYRO_MAG_SQR_SHIFT. This number must be >=0 and even.               */
    /*  The value of inv_accel_sum_of_sqr is scaled such that (1g)^2 =        */
    /*  2^ACC_MAG_SQR_SHIFT                                                   */
    /**************************************************************************/
#define GYRO_MAG_SQR_SHIFT 6
#define ACC_MAG_SQR_SHIFT 16

    /**************************************************************************/
    /*  Prototypes                                                            */
    /**************************************************************************/

    inv_error_t inv_set_fifo_rate(unsigned short fifoRate);
    unsigned short inv_get_fifo_rate(void);
    int_fast16_t inv_get_sample_step_size_ms(void);
    int_fast16_t inv_get_sample_frequency(void);
    long inv_decode_temperature(short tempReg);

    /* Register callbacks after a packet of FIFO data is processed */
    inv_error_t inv_register_fifo_rate_process(inv_obj_func func, int priority);
    inv_error_t inv_unregister_fifo_rate_process(inv_obj_func func);
    inv_error_t inv_run_fifo_rate_processes(void);
    inv_error_t inv_check_fifo_callback(inv_obj_func callback,
        unsigned char *is_registered);

    /* Setup FIFO for various output */
    inv_error_t inv_send_quaternion(uint_fast16_t accuracy);
    inv_error_t inv_send_gyro(uint_fast16_t elements, uint_fast16_t accuracy);
    inv_error_t inv_send_accel(uint_fast16_t elements, uint_fast16_t accuracy);
    inv_error_t inv_send_linear_accel(uint_fast16_t elements,
                                      uint_fast16_t accuracy);
    inv_error_t inv_send_linear_accel_in_world(uint_fast16_t elements,
                                               uint_fast16_t accuracy);
    inv_error_t inv_send_cntrl_data(uint_fast16_t elements,
                                    uint_fast16_t accuracy);
    inv_error_t inv_send_sensor_data(uint_fast16_t elements,
                                     uint_fast16_t accuracy);
    inv_error_t inv_send_external_sensor_data(uint_fast16_t elements,
                                              uint_fast16_t accuracy);
    inv_error_t inv_send_gravity(uint_fast16_t elements,
                                 uint_fast16_t accuracy);
    inv_error_t inv_send_packet_number(uint_fast16_t accuracy);
    inv_error_t inv_send_quantized_accel(uint_fast16_t elements,
                                         uint_fast16_t accuracy);
    inv_error_t inv_send_eis(uint_fast16_t elements, uint_fast16_t accuracy);

    /* Get fixed point data from FIFO */
    inv_error_t inv_get_accel(long *data);
    inv_error_t inv_get_quaternion(long *data);
    inv_error_t inv_get_6axis_quaternion(long *data);
    inv_error_t inv_get_gyro(long *data);
    inv_error_t inv_set_linear_accel_filter_coef(float coef);
    inv_error_t inv_get_linear_accel(long *data);
    inv_error_t inv_get_linear_accel_in_world(long *data);
    inv_error_t inv_get_gyro_and_accel_sensor(long *data);
    inv_error_t inv_get_gyro_sensor(long *data);
    inv_error_t inv_get_gyro_raw(long *data);
    inv_error_t inv_get_cntrl_data(long *data);
    inv_error_t inv_get_temperature(long *data);
    inv_error_t inv_get_gravity(long *data);
    inv_error_t inv_get_unquantized_accel(long *data);
    inv_error_t inv_get_quantized_accel(long *data);
    inv_error_t inv_get_external_sensor_data(long *data, int size);
    inv_error_t inv_get_eis(long *data);
    inv_error_t inv_get_packet_number(uint16_t *data);

    /* Get floating point data from FIFO */
    inv_error_t inv_get_accel_float(float *data);
    inv_error_t inv_get_quaternion_float(float *data);
    inv_error_t inv_get_gyro_raw_float(float *data);

    inv_error_t inv_process_fifo_packet(const unsigned char *dmpData);
    inv_error_t inv_read_and_process_fifo(int_fast8_t numPackets,
                                          int_fast8_t * processed);

    inv_error_t inv_set_fifo_processed_callback(void (*func) (void));

    inv_error_t inv_init_fifo_param(void);
    inv_error_t inv_close_fifo(void);
    inv_error_t inv_set_gyro_data_source(uint_fast8_t source);
    inv_error_t inv_decode_quantized_accel(void);
    unsigned long inv_get_gyro_sum_of_sqr(void);
    unsigned long inv_accel_sum_of_sqr(void);
    void inv_override_quaternion(long *q);
    uint_fast16_t inv_get_fifo_packet_size(void);
    
#ifdef __cplusplus
}
#endif
#endif                          // INVENSENSE_INV_FIFO_H__
