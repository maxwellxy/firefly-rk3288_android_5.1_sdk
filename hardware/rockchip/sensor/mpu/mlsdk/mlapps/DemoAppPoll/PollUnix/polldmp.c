/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
 
/******************************************************************************
 *
 * $Id: polldmp.c 6276 2011-11-09 22:40:46Z mcaramello $
 * 
 *****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>

#include "mltypes.h"
#include "ml.h"
#include "slave.h"
#include "compass.h"
#include "mlFIFO.h"
#include "int.h"
#include "mldl.h"
#include "mlmath.h"
#include "mlsl.h"

#include "temp_comp.h"
#include "mlBiasNoMotion.h"
#include "fastNoMotion.h"
#include "mlsupervisor_9axis.h"

#include "mlsetup.h"
#include "helper.h"

#include "inv_external_slave_ami306.h"
#include "inv_external_slave_akm8975.h"
#include "inv_external_slave_mmc3280.h"

#include "ml_mputest.h"
#include "mputest.c"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "main:"

#define DEBUG_OUT 0

#define TEST_RAW_ACCEL_GYRO   0
#define TEST_COMPASS_ACCURACY  1

float quat[4] = {1.f, 0.f, 0.f, 0.f};

unsigned int flag = 0x82;

/* Motion/no motion callback function */
void onMotion(unsigned short motionType)
{
    switch(motionType) {
        case INV_MOTION:
            printf("Motion\n");
            break;
        case INV_NO_MOTION:
            printf("No Motion\n");
            break;
        default:
            break;
    }

}

void processedData(void)
{
    if (flag & 0x80) {
        float checksum = 0.0;
        float quat[4];
        int i;

        CALL_N_CHECK( inv_get_quaternion_float(quat) );
        for(i = 0; i < 4; i++)
            checksum += (quat[i] * quat[i]);
        MPL_LOGI("%12.4f %12.4f %12.4f %12.4f    -(%12.4f)\n",
                 quat[0], quat[1], quat[2], quat[3], sqrtf(checksum));
    }
}

void dumpData(void)
{
    if (flag & 0x20) {
        float data[6];
        long fixedData[6];
        int ii;
	int accuracy;

#if TEST_COMPASS_ACCURACY
	float magnetic[3];
	CALL_N_CHECK(inv_get_compass_data(fixedData));
	CALL_N_CHECK(inv_get_compass_accuracy(&accuracy));
	
	printf("Magnetic:  %ld  %ld  %ld : accuracy is %d\n", fixedData[0], fixedData[1], fixedData[2], accuracy);
//	printf("Magnetic:  %12.4f %12.4f %12.4f : accuracy is %d\n", magnetic[0], magnetic[1], magnetic[2], accuracy);

#endif
	
#if TEST_RAW_ACCEL_GYRO   
        CALL_N_CHECK( inv_get_accel(fixedData) );
        CALL_N_CHECK( inv_get_gyro(&fixedData[3]) );
        for (ii = 0; ii < 6; ii++) {
            data[ii] = fixedData[ii] / 65536.0f;
        }
        printf("A: %12.4f %12.4f %12.4f G: %12.4f %12.4f %12.4f \n",
                 data[0], data[1], data[2], data[3], data[4], data[5]);
#endif
    }
}

// Main function
int main(int argc, char *argv[])
{            
    //unsigned short accelSlaveAddr = ACCEL_SLAVEADDR_INVALID;
    unsigned short platformId = ID_INVALID;
    unsigned short accelId = ID_INVALID;
    unsigned short compassId = ID_INVALID;
    unsigned char reg[32];
    unsigned char *verStr;
    int result;
    int key = 0;
    int interror;
    struct mpuirq_data **data;
    const char *ints[] = { "/dev/mpuirq",        /* INTSRC_MPU  */
                           //"/dev/accelirq",    /* INTSRC_AUX1 */
                           //"/dev/compassirq",  /* INTSRC_AUX2 */
                           //"/dev/pressureirq", /* INTSRC_AUX3 */
    };
    int handles[ARRAY_SIZE(ints)];

    CALL_N_CHECK( inv_get_version(&verStr) );
    printf("%s\n", verStr);

    if(INV_SUCCESS == MenuHwChoice(&platformId, &accelId, &compassId)) {
        CALL_CHECK_N_RETURN_ERROR(SetupPlatform(platformId,
                                                accelId, compassId));
    }

    CALL_CHECK_N_RETURN_ERROR( inv_serial_start("/dev/mpu") );
    
    IntOpen(ints, handles, ARRAY_SIZE(ints));
    if (handles[0] < 0) {
        printf("IntOpen failed\n");
        interror = INV_ERROR;
    } else {
        interror = INV_SUCCESS;
    }

    CALL_CHECK_N_RETURN_ERROR( inv_dmp_open() );
    CALL_CHECK_N_RETURN_ERROR(inv_set_mpu_sensors(INV_NINE_AXIS));

    /***********************/
    /* advanced algorithms */
    /***********************/
    /* The Aichi and AKM libraries are only built against the
     * android tool chain */
    CALL_CHECK_N_RETURN_ERROR(inv_enable_bias_no_motion());
    CALL_CHECK_N_RETURN_ERROR(inv_enable_bias_from_gravity(true));
    CALL_CHECK_N_RETURN_ERROR(inv_enable_bias_from_LPF(true));
    CALL_CHECK_N_RETURN_ERROR(inv_set_dead_zone_normal(true));
#ifdef ANDROID
    {
        struct mldl_cfg *mldl_cfg = inv_get_dl_config();
        if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS] &&
            mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id == COMPASS_ID_AK8975) {
            CALL_CHECK_N_RETURN_ERROR(inv_enable_9x_fusion_external());
            CALL_CHECK_N_RETURN_ERROR(inv_external_slave_akm8975_open());
        } else if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS] &&
                   mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id ==
                   COMPASS_ID_AMI306) {
            CALL_CHECK_N_RETURN_ERROR(inv_enable_9x_fusion_external());
            CALL_CHECK_N_RETURN_ERROR(inv_external_slave_ami306_open());
        } else if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS] &&
                   mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id ==
                   COMPASS_ID_MMC328X) {
              
	 printf("Compass id is %d\n", mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id);
            CALL_CHECK_N_RETURN_ERROR(inv_enable_9x_fusion_external());
            CALL_CHECK_N_RETURN_ERROR(inv_external_slave_mmc3280_open());
       }else {
            CALL_CHECK_N_RETURN_ERROR(inv_enable_9x_fusion());
        }
    }
#else
    CALL_CHECK_N_RETURN_ERROR(inv_enable_9x_fusion());
#endif
    CALL_CHECK_N_RETURN_ERROR( inv_enable_temp_comp() );
    CALL_CHECK_N_RETURN_ERROR( inv_enable_fast_nomot() );

    CALL_CHECK_N_RETURN_ERROR( inv_set_motion_callback(onMotion) );
    CALL_CHECK_N_RETURN_ERROR( inv_set_fifo_processed_callback(processedData) );

    /* Setup FIFO */
    CALL_CHECK_N_RETURN_ERROR( inv_send_quaternion(INV_32_BIT) );
    CALL_CHECK_N_RETURN_ERROR( inv_send_gyro(INV_ALL,INV_32_BIT) );
    CALL_CHECK_N_RETURN_ERROR( inv_send_accel(INV_ALL,INV_32_BIT) );
    CALL_CHECK_N_RETURN_ERROR( inv_set_fifo_rate(20) );

    /* Check to see if interrupts are available.  If so use them */
    if (INV_SUCCESS  ==  interror) {
        CALL_CHECK_N_RETURN_ERROR( inv_set_fifo_interrupt(true) );
        CALL_CHECK_N_RETURN_ERROR( inv_set_motion_interrupt(true) );
        CALL_CHECK_N_RETURN_ERROR( IntSetTimeout(handles[0], 100) );
        MPL_LOGI("Interrupts Configured\n");
        flag |= 0x04;
    } else {
        MPL_LOGI("Interrupts unavailable on this platform\n");
        flag &= ~0x04;
    }

    CALL_CHECK_N_RETURN_ERROR( inv_dmp_start() );

    //Loop  
    while (1) {

        usleep(8000);

        result = ConsoleKbhit();
        if (DEBUG_OUT)
            printf("_kbhit result : %d\n", result);
        if (result) {
            key = ConsoleGetChar();
            if (DEBUG_OUT)
                printf("getchar key : %c (%d)\n", key, key);
        } else{
            key = 0; 
        } 

        if (key == 'q') {
            printf("quit...\n");
            break;
        } else if (key == '0') {
            printf("flag=0\n");
            flag = 0; 
        } else if (key == '1') {
            if (flag & 1) {
                MPL_LOGI("flag &= ~1 - who am i\n");
                flag &= ~1;
            } else {
                MPL_LOGI("flag |= 1 - who am i\n");
                flag |= 1;
            }
        } else if (key == '2') {
            if (flag & 2) {
                MPL_LOGI("flag &= ~2 - inv_update_data()\n");
                flag &= ~2;
            } else {
                MPL_LOGI("flag |= 2 - inv_update_data()\n");
                flag |= 2;
            }
        } else if (key == '4') {
            if (flag & 4) {
                MPL_LOGI("flag &= ~4 - IntProcess()\n");
                flag &= ~4;
            } else {
                MPL_LOGI("flag |= 4 - IntProcess()\n");
                flag |= 4;
            }
        } else if (key == 'a') {
            if (flag & 0x80) {
                MPL_LOGI("flag &= ~0x80 - Quaternion\n");
                flag &= ~0x80;
            } else {
                MPL_LOGI("flag |= 0x80  - Quaternion\n");
                flag |= 0x80;
            }
        } else if (key == 'b') {
            if (flag & 0x20) {
                printf("flag &= ~0x20 - dumpData()\n");
                flag &= ~0x20;
            } else {
                printf("flag |= 0x20 - dumpData()\n");
                flag |= 0x20;
            }
        } else if (key == 'S') {
            MPL_LOGI("run MPU Self-Test...\n");
            CALL_CHECK_N_RETURN_ERROR(inv_self_test_run());
            inv_sleep(5);
            continue;
        } else if (key == 'C') {
            MPL_LOGI("run MPU Calibration Test...\n");
            CALL_CHECK_N_RETURN_ERROR(inv_self_test_calibration_run());
            inv_sleep(5);
            continue;
        } else if (key == 'Z') {
            MPL_LOGI("run MPU Self-Test for Accel Z-Axis...\n");
            CALL_CHECK_N_RETURN_ERROR(inv_self_test_accel_z_run());
            inv_sleep(5);
            continue;
        } else if (key == 'h') {
            printf(
                "\n\n"
                "0   -   turn all the features OFF\n"
                "1   -   read WHO_AM_I\n"
                "2   -   call inv_update_data()\n"
                "4   -   call IntProcess()\n"
                "a   -   print Quaternion data\n"
                "b   -   Print raw accel and gyro data\n"
                "S   -   interrupt execution, run self-test\n"
                "C   -   interrupt execution, run calibration test\n"
                "Z   -   interrupt execution, run accel Z-axis test\n"
                "h   -   show this help\n"
                "\n\n"
                );
        } 

        if (flag & 0x01) {
            if (DEBUG_OUT)
                printf("inv_serial_readSingle(0x68,0,reg)\n");
            CALL_CHECK_N_RETURN_ERROR( 
                inv_serial_read(inv_get_serial_handle(), 0x68, 0, 1, reg) );
            printf("\nreg[0]=%02x", reg[0]);
        }
        if (flag & 0x02) {
            CALL_N_CHECK( inv_update_data() );
        }
        if (flag & 0x04) {
            data = InterruptPoll(handles, ARRAY_SIZE(handles), 0, 500000);
            InterruptPollDone(data);
        }
        if (flag & 0x20) {
            dumpData();
        }
    }    

    // Close Motion Library
    CALL_CHECK_N_RETURN_ERROR( inv_dmp_close() );
    CALL_CHECK_N_RETURN_ERROR( inv_serial_stop() );

    CALL_N_CHECK(IntClose(handles, ARRAY_SIZE(handles)));

    return INV_SUCCESS;
}

