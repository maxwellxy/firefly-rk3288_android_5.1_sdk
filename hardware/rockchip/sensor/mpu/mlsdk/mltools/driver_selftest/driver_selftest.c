/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: driver_selftest.c 5657 2011-06-17 20:22:03Z nroyer $
 * 
 *****************************************************************************/

/**
 *  @defgroup DriverSelftest
 *  @brief  A small self-test application targeted at helping system 
 *          integrators verify that the MPL driver layer is functioning.
 *
 *  @{
 *      @file   driver_selftest.c
 *      @brief  Hardware self testing.
 *              To use this application, link with the MPL and the 
 *              implementation of MLSL.  
 *              See the usage documentation for command line arguments. The 
 *              output of this program is saved in a file (test_report.txt) 
 *              and is simultaneously printed to the console.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include "helper.h"  /* for findComm */
#endif

#include "mldl.h"
#include "mpu.h"
#include "mlmath.h"

#include "mlsl.h"
#include "mlos.h"

#include "gopt.h"

/*
    Defines
*/

//#define LINUX_GPIO
#define WHOAMI_MASK (0x7e)

/* static storage for strings */
#define N_MSGS      1000
#define MAX_MSG_LEN 1024

typedef struct tMsgBufs {
    int num_msgs;
    char msg[N_MSGS][MAX_MSG_LEN + 1];
} tMsgBufs;

char desc_buf  [MAX_MSG_LEN + 1];    // test description strin
char error_buf [MAX_MSG_LEN + 1];    // test error message
tMsgBufs msg_bufs;                   // test additional messages


/* Global config data */
static char g_port[256] = "/dev/mpu";
void *mlsl_handle;

// flags
int g_verbose = false;
int g_fail_on_error = false;
int g_continuous_run = false;

// DEFAULTS
// mpu
int g_mpu_slaveaddr = 0x68;
int g_whoami = 0x68;
// accel
int g_accel_slaveaddr = 0x0f;
int g_accel_burstaddr = 0x06;
int g_accel_burstlen = 6;

/* utility functions */
int parse_commandline(int argc, const char **argv);

/* test functions */
typedef struct {
    int res;
    char *desc;
    char *error;
    tMsgBufs *messages;
} test_result;

typedef test_result (*test_func) (void);

test_result test_open(void);
test_result test_single_wr(void);
test_result test_burst_wr(void);
test_result test_single_read(void);
test_result test_burst_read(void);
test_result test_single_write(void);
test_result test_burst_write(void);
test_result test_register_dump(void);
test_result test_fill_ram(void);
test_result test_fill_ram_in_chunks(void);
test_result test_fill_fifo(void);
test_result test_bypass(void);
test_result test_gyro_sanity(void);
test_result test_accel_sanity(void);
test_result test_close(void);

test_func test_funcs[] = { 
    test_open, 
    test_single_wr,
    test_burst_wr,
    test_single_write,
    test_single_read,
    test_burst_write,
    test_burst_read,
    test_register_dump,
    test_fill_ram,
    test_fill_ram_in_chunks,
    test_fill_fifo,
    test_bypass, 
    test_gyro_sanity, 
    //test_accel_sanity,
    test_close
};


/*
    Macros
*/

#define OUTPUT(s,fp)                         \
    fputs(s, fp); fputs("\n", fp);           \
    puts(s);

#define RESULT_SETUP(str)           \
    test_result r = {               \
        /*.res =      */ 1,         \
        /*.desc =     */ desc_buf,  \
        /*.error =    */ error_buf, \
        /*.messages = */ &msg_bufs  \
    };                              \
    r.messages->num_msgs = 0;       \
    sprintf(r.desc, "%s", str);

#define RETURN_SUCCESS(r)    \
    r.res = 1;               \
    return r;

#define RECORD_ERROR(result, ...)                       \
    if (result != INV_SUCCESS) {                         \
        r.res = 0;                                      \
        sprintf(r.error, __VA_ARGS__);                  \
        sprintf(r.error, "%s (ec:%d)", r.error, result);\
        return r;                                       \
    }

#define RECORD_ERROR_COND(cond, ...)                    \
    if (cond) {                                         \
        r.res = 0;                                      \
        sprintf(r.error, __VA_ARGS__);                  \
        return r;                                       \
    }

#define ADD_MESSAGE(...)                                           \
    if(msg_bufs.num_msgs<N_MSGS)                                   \
        sprintf(msg_bufs.msg[msg_bufs.num_msgs++],__VA_ARGS__);


/*
    Functions
*/

void print_data(unsigned char* data, unsigned short length)
{
#define WRAP_LIMIT 16
    char line[MAX_MSG_LEN];
    const char pad[10] = "";
    int i;

    // heading
    sprintf(line, "%s      ", pad);
    for (i= 0; i < MIN(WRAP_LIMIT, length); i++) {
        sprintf(line, "%s%2x ", line, i);
    }
    ADD_MESSAGE("%s", line);

    sprintf(line, "%s      ", pad);
    for ( i= 0; i < MIN(WRAP_LIMIT, length); i++) {
        sprintf(line, "%s|  ", line);
    }
    ADD_MESSAGE("%s", line);

    // data
    line[0] = 0;
    for (i = 0; i < length; i++) {
        if (i % WRAP_LIMIT == 0) {
            if (i > 0) {
                // end line
                ADD_MESSAGE("%s", line);
            }
            // start a new line 
            sprintf(line, "%s%-2x - ", pad, i / WRAP_LIMIT);
        }
        sprintf(line, "%s%02X ", line, data[i]);
    }
    ADD_MESSAGE("%s", line);
}



int gpio_set_direction(int pin, int in)
{
    FILE *fp;
    char set_value[4]; 
    char direction_file[128];

    // Assumes that gpio_setup has already been called
    // SET DIRECTION
    // Open the LED's sysfs file in binary for reading and writing, 
    // store file pointer in fp
    sprintf(direction_file, "/sys/class/gpio/gpio%d/direction", pin);
    if ((fp = fopen(direction_file, "rb+")) == NULL) {
        printf("Cannot open direction file.\n");
        return -1;
    }
    //Set pointer to begining of the file
    rewind(fp);
    //Write our value of "in" or "out" to the file
    if (in) {
        strcpy(set_value, "in");
    } 
    else {
        strcpy(set_value, "out");
    }
    fwrite(&set_value, sizeof(char), strlen(set_value), fp);
    fclose(fp);
    
    printf("...direction set to %d\n", in);
    return 0;
}

int gpio_set_value(int pin, int value)
{   
    // SET VALUE
    // Open the LED's sysfs file in binary for reading and writing, 
    // store file pointer in fp

    FILE *fp;
    char set_value[4]; 
    char value_file[128];

    sprintf(value_file, "/sys/class/gpio/gpio%d/value", pin);
    if ((fp = fopen(value_file, "rb+")) == NULL) {
        printf("Cannot open value file.\n");
        return -1;
    }
    //Set pointer to begining of the file
    rewind(fp);
    //Write our value of "1" to the file 
    if (value) {
        strcpy(set_value, "1");
    } 
    else {
        strcpy(set_value, "0");
    }
    fwrite(&set_value, sizeof(char), 1, fp);
    fclose(fp);
    printf("...value set to %d...\n", value);
    return 0;
}

int gpio_setup(int pin, int in, int value)
{
    FILE *fp;
    //create a variable to store whether we are sending a '1' or a '0'
    char set_value[4]; 
    
    if (pin > 0xFF) {
        return -2;
    }
    printf("\n"
           "**********************************\n"
           "*  Configuring GPIO %d  *\n"
           "**********************************\n", pin);
    
    //Using sysfs we need to write "37" to /sys/class/gpio/export
    //This will create the folder /sys/class/gpio/gpio37
    if ((fp = fopen("/sys/class/gpio/export", "ab")) == NULL)
    {
        printf("Cannot open export file.\n");
        return -1;
    }
    //Set pointer to begining of the file
    rewind(fp);
    //Write our value of "37" to the file
    sprintf(set_value,"%d", pin & 0xff);
    fwrite(&set_value, sizeof(char), strlen(set_value), fp);
    fclose(fp);
    
    printf("%s...export file accessed, new pin now accessible\n", set_value);
    
    if (gpio_set_direction(pin, in)) return -2;
    if (gpio_set_value(pin, value)) return -3;

    return 0;
}

/*
    Data generation functions
*/

typedef void (*data_setup_func) (unsigned char*, int);

void setup_zero(unsigned char* data, int len) 
{
    memset(data, 0x00, len);
}

void setup_ff(unsigned char* data, int len) 
{
    memset(data, 0xff, len);
}

void setup_aa(unsigned char* data, int len) 
{
    memset(data, 0xaa, len);
}

void setup_aa_55(unsigned char* data, int len) 
{
    int ii;
    for (ii = 0; ii + 1 < len; ii += 2) {
        data[ii] = 0xaa;
        data[ii+1] = 0x55;
    }
}

void setup_counter(unsigned char* data, int len) 
{
    int i;
    for(i = 0; i < len; i++) {
        data[i] = i & 0xFF;
    }
}

/*--- Unif (1,254) ---*/
void setup_random(unsigned char* data, int len) 
{
    int i;
    for(i = 0; i < len; i++) {
        data[i] = (1 + rand()) & (0xFF - 2);
    }
}

void error_pattern(unsigned char* data, int len) 
{
    memset(data, 0xBE, len);
}



/*
    Other
*/

int parse_commandline(int argc, const char** argv) 
{
#if !defined(WIN32)
    const char *val;
    const void *options = gopt_sort( 
       &argc, argv, 
       gopt_start(
           gopt_option('h', 0, gopt_shorts('h', '?'), 
                                gopt_longs("help", "HELP")),
           gopt_option('v', 0, gopt_shorts('v' ), 
                                gopt_longs("verbose", "VERBOSE")),
           gopt_option('f', 0, gopt_shorts('f'), 
                                gopt_longs("fail", "FAIL")),
           gopt_option('c', 0, gopt_shorts('c'), 
                                gopt_longs("continuous", "CONTINUOUS")),
           gopt_option('p', GOPT_ARG, gopt_shorts('p'), 
                                       gopt_longs("port")),
           gopt_option('m', GOPT_ARG, gopt_shorts('m'), 
                                       gopt_longs("mpuslaveaddr")),
           gopt_option('a', GOPT_ARG, gopt_shorts('a'), 
                                       gopt_longs("accelslaveaddr")),
           gopt_option('d', GOPT_ARG, gopt_shorts('d'), 
                                       gopt_longs("accelburstaddr")),
           gopt_option('l', GOPT_ARG, gopt_shorts('l'), 
                                       gopt_longs("accelburstlen")),
           gopt_option('w', GOPT_ARG, gopt_shorts('w'), 
                                       gopt_longs("whoami"))
       )
    );

    if(gopt(options, 'h')) {
      /*
       * if any of the help options was specified
       */
      fprintf(stdout, "MLSL Serial Driver Self Test\n");
      fprintf(stdout, "usage: %s "
                       "[-h] [-v] [-f] [-c] "
                       "[-p port/device_driver] "
                       "[-m addr] [-a addr] [-d reg] [-l len] [-w val] \n", 
                       argv[0]);
      fprintf(stdout, "\noptions:\n");
      fprintf(stdout, "\t-p\tport / device driver\n");
      fprintf(stdout, "\t-h\tprint help message\n");
      fprintf(stdout, "\t-v\tuse verbose output\n"); 
      fprintf(stdout, "\t-f\tabort on failed test\n"); 
      fprintf(stdout, "\t-c\trun continuosly, terminate with <CTRL+C>\n");
      fprintf(stdout, "\t-m\tmpu slave address\n");
      fprintf(stdout, "\t-w\texpected value of mpu whoami register\n");
      fprintf(stdout, "\t-a\taccel slave address\n");
      fprintf(stdout, "\t-d\tfirst data register on accelerometer\n");
      fprintf(stdout, "\t-l\tnumber of accelerometer data registers\n");
      exit(0);
    }
    if(gopt(options, 'v')) {
        g_verbose = true;
        fprintf(stdout, "Using 'verbose' output\n");
    }
    if(gopt(options, 'f')) {
        g_fail_on_error = true;
        fprintf(stdout, "Using 'fail' on error\n");
    }
    if(gopt(options, 'c')) {
        g_continuous_run = true;
        fprintf(stdout, "Using 'continuous' run. Use <CTRL+C> to terminate.\n");
    }
    if(gopt_arg(options, 'p', &val)) { sscanf(val,"%s",g_port); }
    if(gopt_arg(options, 'm', &val)) { sscanf(val,"%x",&g_mpu_slaveaddr); }
    if(gopt_arg(options, 'a', &val)) { sscanf(val,"%x",&g_accel_slaveaddr); }
    if(gopt_arg(options, 'w', &val)) { sscanf(val,"%x",&g_whoami); }
    if(gopt_arg(options, 'd', &val)) { sscanf(val,"%x",&g_accel_burstaddr); }
    if(gopt_arg(options, 'l', &val)) { sscanf(val,"%x",&g_accel_burstlen); }
#else
    if (argc >= 2) {
        strcpy(g_port, argv[1]);
    }
    else {
        return findComm(g_port, sizeof(g_port));
    }
#endif

    return 1;
}




/*******************************************************************
    TEST CASES definitions
*******************************************************************/

#define CHECK_RES                               \
    if(res!=INV_SUCCESS) {                       \
        printf("ERROR @ line %d\n",__LINE__);   \
        goto done;                              \
    }                                           \

test_result test_open(void) 
{
    unsigned short res;
    
    RESULT_SETUP("Open Serial Port");

    res = inv_serial_open(g_port, &mlsl_handle);
    RECORD_ERROR(res, "serial open failed");

    // reset and wake up the chip. 
    //  It may fail if the driver is not actually working
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_PWR_MGM, 0xC0);
    RECORD_ERROR(res, "serial open failed - could not reset the part");
    inv_sleep(5);

    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_PWR_MGM, 0x00);
    RECORD_ERROR(res, "serial open failed - could not wake up the part");
    inv_sleep(5);

    RETURN_SUCCESS(r);
}

test_result test_close(void) 
{
    unsigned short res;
    unsigned char user_ctrl;

    RESULT_SETUP("Close Serial Port");

    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_USER_CTRL, 1, &user_ctrl);
    CHECK_RES;

    if (user_ctrl & BIT_AUX_IF_EN) {  // it's in master mode
        // have the controller get a NACK
        res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                    MPUREG_AUX_SLV_ADDR, 0x7F);
        CHECK_RES;
        inv_sleep(5);
        // enable bypass
        res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                    MPUREG_USER_CTRL, 0x00);
        CHECK_RES;
        inv_sleep(5);
    }
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_PWR_MGM, 0x40);
    RECORD_ERROR(res, "serial open failed - could not sleep the part");

    res = inv_serial_close(mlsl_handle);

done:
    RECORD_ERROR(res, "serial close failed");
    RETURN_SUCCESS(r);
}

test_result test_single_read(void)
{
    unsigned short res;
    unsigned char data;

    RESULT_SETUP("Single byte read");

    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 0x00, 1, &data);
    RECORD_ERROR(res, "serial single byte read failed");
    
    if ((data & WHOAMI_MASK) != (g_whoami & WHOAMI_MASK)) {
        res = 1;
        RECORD_ERROR(res, 
                     "WHO_AM_I register unexpected val: %hhx (expected %x)", 
                     data, g_whoami);
    }
    else if (g_verbose) {
        ADD_MESSAGE("WHO_AM_I register is 0x%02X (masked 0x%02X)", 
                    data, data & WHOAMI_MASK);
    }

    RETURN_SUCCESS(r);
}


test_result test_register_dump(void) 
{
    unsigned char data;
    unsigned short res = 0;
    unsigned char ff_cnt = 0;
    unsigned int error = false;
    unsigned char usrCfgRegs[NUM_OF_MPU_REGISTERS];
    int ii;

    RESULT_SETUP("Register dump");

    for(ii = 0; ii < NUM_OF_MPU_REGISTERS; ii++) {
        if (ii == MPUREG_FIFO_R_W || ii == MPUREG_MEM_R_W) {
            data = 0x00;
        }
        else {
            res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, ii, 1, &data);
            RECORD_ERROR(res, "register dump failed");
        }
        ff_cnt += (data == 0xff);
        usrCfgRegs[ii] = data;
    }
    if (ff_cnt > NUM_OF_MPU_REGISTERS/4) {
        error = true;
    }

    print_data(usrCfgRegs, NUM_OF_MPU_REGISTERS);
    ADD_MESSAGE(" ");

    RECORD_ERROR(error, "register dump failed (too many I2C NACKs)");
    RECORD_ERROR(res,   "register dump failed");

    RETURN_SUCCESS(r);
    
}


test_result test_burst_read(void) 
{
    unsigned char data[6];
    unsigned short res;
    int i;

    RESULT_SETUP("Multi byte read");

    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_GYRO_XOUT_H, 6, data);
    RECORD_ERROR(res, "serial burst read failed - serial communication");

    for(i = 0; i < 5; i++) {
        if(data[i] == data[i + 1] ) {
            res = 1;
            break;
        }
    }

    RECORD_ERROR(
        res, 
        "serial burst read failed - data unchanged "
        "(%02X %02x %02x %02x %02x %02x)", 
        data[0], data[1], data[2], data[3], data[4], data[5]
    );
    RETURN_SUCCESS(r);
}


test_result test_single_write(void)
{
    unsigned char data = BIT_DMP_EN|BIT_FIFO_EN;
    unsigned char usr_ctrl_before = 0x00;
    unsigned char usr_ctrl_after = 0x00;
    //unsigned char data = 0xDF;    // reset all also
    unsigned short res = INV_SUCCESS;

    RESULT_SETUP("Single byte write");

    // Set slave address and burst addr
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_AUX_SLV_ADDR, g_accel_slaveaddr);
    CHECK_RES;
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_ACCEL_BURST_ADDR, g_accel_burstaddr);
    CHECK_RES;
    inv_sleep(1);

    // Enable the DMP, FIFO and auxiliary slave (master mode)
    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_USER_CTRL, 1, &usr_ctrl_before);
    CHECK_RES;
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_USER_CTRL, data);
    CHECK_RES;
    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_USER_CTRL, 1, &usr_ctrl_after);
    CHECK_RES;

    //expected value is not 0xff as some bits reset automatically
    RECORD_ERROR_COND(
        usr_ctrl_after != (data & 0xF0), 
        "serial write failed : USER_CTRL reg is 0x%02X (exp:0x%02X)", 
        usr_ctrl_after, data & 0xF0
    );


    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_USER_CTRL, usr_ctrl_before);
    CHECK_RES;
    inv_sleep(1);
    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_USER_CTRL, 1, &data);
    CHECK_RES;

    RECORD_ERROR_COND(
        usr_ctrl_before!=data, 
        "single write failed - USR_CTRL reg is 0x%02X (exp:0x%02X)",
        data, usr_ctrl_before
    );

done:
    RECORD_ERROR(res, "single write failed - serial communication");
    RETURN_SUCCESS(r);
}

test_result test_burst_write(void)
{
    unsigned char data[7];
    unsigned char *write_data = &data[1];
    unsigned char read_data[6];
    unsigned short res;
    int i;

    RESULT_SETUP("Multi byte write");

    data[0] = MPUREG_X_OFFS_USRH;
    for(i = 0; i < 6; i++) {
        data[i + 1] = i;
    }
    res = inv_serial_write(mlsl_handle, g_mpu_slaveaddr, 
                          7, data);
    CHECK_RES;
    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         data[0], 6, read_data);
    CHECK_RES;

    res = 0;
    for(i = 0; i < 6; i++) {
        if(write_data[i] != read_data[i]) {
            res++;
        }
    }
    RECORD_ERROR(res,
                 "serial burst write failed : "
                 "%02X-%02X %02X-%02X %02X-%02X %02X-%02X %02X-%02X %02X-%02X",
                 write_data[0], read_data[0], write_data[1], read_data[1], 
                 write_data[2], read_data[2], write_data[3], read_data[3], 
                 write_data[4], read_data[4], write_data[5], read_data[5]
    );

done:
    RECORD_ERROR(res, "single write failed - serial communication");
    RETURN_SUCCESS(r);
}

test_result test_single_wr(void)
{
    unsigned short res;
    unsigned char write_val = rand() % 256;
    unsigned char read_val;

    RESULT_SETUP("Single byte write and read");

    // Set slave address and burst addr
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_AUX_SLV_ADDR, write_val);
    CHECK_RES;
    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_AUX_SLV_ADDR, 1, &read_val);

    RECORD_ERROR_COND(
        read_val != write_val, 
        "serial write failed : "
        "AUX_SLV_ADDR reg is 0x%02X (exp:0x%02X)", 
        read_val, write_val
    );
    // restore reg value
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_AUX_SLV_ADDR, g_accel_slaveaddr);
    CHECK_RES;

done:
    RECORD_ERROR(res, "single wr failed - serial communication");
    RETURN_SUCCESS(r);
}


#define REG_BURST_START MPUREG_PRODUCT_ID
#define REG_BURST_LEN 20
test_result test_burst_wr(void)
{
    unsigned char i2c_write[1 + REG_BURST_LEN];
    unsigned char *write_data = &i2c_write[1];
    unsigned char read_data[REG_BURST_LEN];
    unsigned short thisLen;
    unsigned short res, fail;
    int i;
    
    RESULT_SETUP("Multi byte write and read");

    i2c_write[0] = REG_BURST_START;  
    setup_random(write_data, REG_BURST_LEN);
    //write_data[0] = DEFAULT_MPU_SLAVEADDR;

    if (g_verbose) {
        ADD_MESSAGE("Write data:");
        print_data(write_data, REG_BURST_LEN);
    }

    thisLen = 1;
    while (thisLen < REG_BURST_LEN) {
        setup_ff(read_data, REG_BURST_LEN);  // clear read data from prev cycle

        if (g_verbose) {
            ADD_MESSAGE("using length %d", thisLen);
        }
        res = inv_serial_write(mlsl_handle, g_mpu_slaveaddr, 
                              1 + thisLen, i2c_write);
        CHECK_RES;
        res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                             REG_BURST_START, 
                             thisLen, read_data);
        CHECK_RES;
        // error checking
        for(i = 0, fail = 0; i < thisLen; i++) {
            if(write_data[i] != read_data[i]) {
                fail++;
            }
        }
        // error handling
        if (fail) {
            if (g_verbose) {
                ADD_MESSAGE("Write data");
                print_data(write_data, thisLen);
                ADD_MESSAGE("Read data");
                print_data(read_data, thisLen);
            }
            RECORD_ERROR(fail, 
                         "serial burst write and read failed at %d B", 
                         thisLen );
        }
    
        thisLen += 1;
    }

done:
    // reset MPU
    inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                          MPUREG_PWR_MGM, 0x80);
    RECORD_ERROR(res, "burst write and read failed - serial communication");
    RETURN_SUCCESS(r);
}
#ifdef WIN32
//#define FILL_RAM_SIZE (MPU_MEM_BANK_SIZE)
#define FILL_RAM_SIZE (128) // ok
#else
#define FILL_RAM_SIZE (MPU_MEM_BANK_SIZE)
#endif


test_result test_fill_ram_in_chunks(void)
{
    unsigned char write_data[MPU_MEM_BANK_SIZE], read_data[MPU_MEM_BANK_SIZE];
    unsigned short res;
    const unsigned char bank = MPU_MEM_RAM_BANK_1;
    unsigned short chunk_size = MPU_MEM_BANK_SIZE * 2; // will be divided by 2
    int fail = true;
    int i;

    RESULT_SETUP("Load RAM bank");

    setup_counter(write_data, MPU_MEM_BANK_SIZE);

    // try and load memory halving the chunk size at each try
    while(fail) {
        unsigned rem = MPU_MEM_BANK_SIZE;

        chunk_size /= 2;
        while(rem) {
            const unsigned char memStart = MPU_MEM_BANK_SIZE-rem; 
            //write to memory area
            res = inv_serial_write_mem(mlsl_handle, g_mpu_slaveaddr,
                                     bank << 8 | memStart, 
                                     chunk_size, &write_data[memStart]);
            CHECK_RES;

            //read back
            res = inv_serial_read_mem(mlsl_handle, g_mpu_slaveaddr,
                                    bank << 8 | memStart, 
                                    chunk_size, &read_data[memStart]);
            CHECK_RES;
            rem -= chunk_size;
        }

        if (g_verbose) {
            ADD_MESSAGE("Written data");
            print_data(write_data, MPU_MEM_BANK_SIZE);
            ADD_MESSAGE("Read data");            
            print_data(read_data, MPU_MEM_BANK_SIZE);
        }

        fail = false;
        for (i = 0; i < MPU_MEM_BANK_SIZE; i++) {
            if (write_data[i] != read_data[i]) {
                fail = true;
            }
        }
    }

done:
    RECORD_ERROR(res,  "RAM bank fill failed - serial communication");
    RECORD_ERROR(fail, "RAM bank fill failed - serial communication");

    ADD_MESSAGE("RAM bank fill succeeded with %d B chunks", chunk_size);
    RETURN_SUCCESS(r);
}


test_result test_fill_ram(void)
{
    unsigned char write_data[MPU_MEM_BANK_SIZE], read_data[MPU_MEM_BANK_SIZE];
    unsigned short res;
    const unsigned char bank = MPU_MEM_RAM_BANK_1;
    int fail = false;
    data_setup_func fill_data[] = { 
        setup_zero, 
        setup_ff, 
        setup_aa, 
        setup_aa_55,
        setup_counter 
    };
    const int num_data_sets = sizeof(fill_data) / sizeof(data_setup_func);
    int i, j;

    RESULT_SETUP("Fill RAM area");

    for(i = 0; i < num_data_sets; i++) {
        fill_data[i](write_data, FILL_RAM_SIZE);
        error_pattern(read_data, FILL_RAM_SIZE);

        //write to memory area
        res = inv_serial_write_mem(mlsl_handle, g_mpu_slaveaddr,
                                 bank << 8 | 0x00, FILL_RAM_SIZE, write_data);
        CHECK_RES;

        //read back
        res = inv_serial_read_mem(mlsl_handle, g_mpu_slaveaddr,
                                bank << 8 | 0x00, FILL_RAM_SIZE, read_data);
        CHECK_RES;

        if (g_verbose) {
            ADD_MESSAGE("Written data");
            print_data(write_data, FILL_RAM_SIZE);
            ADD_MESSAGE("Read data");            
            print_data(read_data, FILL_RAM_SIZE);
            ADD_MESSAGE(" ");            
        }

        //check
        for(j = 0; j < FILL_RAM_SIZE; j++){
            if(write_data[j] != read_data[j]) {
                if (g_verbose) {
                    ADD_MESSAGE("RAM area fill mismatch : %02X -> %02X != %02X", 
                            j, (int)write_data[j], (int)read_data[j]);
                }
                fail++;
            }
        }
        RECORD_ERROR_COND(
            fail > 0,
            "RAM area fill failed - "
            "memory write and read mismatch in %d locations", 
            fail);
    }

done:
    RECORD_ERROR(res, "RAM area fill failed - serial communication");
    RETURN_SUCCESS(r);
}


#define FILL_FIFO_SIZE      (512)
#define FIFO_RW_CHUNK_SIZE  (128)

test_result test_fill_fifo(void) 
{
    unsigned char write_data[FILL_FIFO_SIZE], read_data[FILL_FIFO_SIZE];
    unsigned char usr_ctrl;
    unsigned short res;
    //int fail = 0;
    int len = 0xff;
    int tries = 0;
    int mismatches = 0;
    int j;

    RESULT_SETUP("Fill FIFO");
    
    j = 0;
    setup_counter(write_data, FILL_FIFO_SIZE);
    setup_zero(read_data, FILL_FIFO_SIZE);

    if (g_verbose) {
        ADD_MESSAGE("Written data");
        print_data(write_data, FILL_FIFO_SIZE);
        ADD_MESSAGE(" ");
    }

    res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                         MPUREG_USER_CTRL, 1, &usr_ctrl);
    CHECK_RES;

    // fifo reset
    while (len > 0  && tries++ < 6) {
        res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                    MPUREG_USER_CTRL, 
                                    (usr_ctrl & (~BIT_FIFO_EN)) | BIT_FIFO_RST); 
        CHECK_RES;
        inv_sleep(5);
        res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr,
                             MPUREG_FIFO_COUNTH, 2, read_data);
        CHECK_RES;
        len = read_data[0] << 8 | read_data[1];
    }
    RECORD_ERROR_COND(tries >= 6, "FIFO reset failed - length is %d", len);

    //set up write to FIFO
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_USER_CTRL, 
                                usr_ctrl | (BIT_FIFO_EN | BIT_FIFO_RST));
    CHECK_RES;
    inv_sleep(5);

    // write the fifo, FIFO_RW_CHUNK_SIZE bytes at a time
    len = 0;
    while (len < FILL_FIFO_SIZE) {
        res = inv_serial_write_fifo(mlsl_handle, g_mpu_slaveaddr, 
                                  FIFO_RW_CHUNK_SIZE, &write_data[len]);
        CHECK_RES;        
        len += FIFO_RW_CHUNK_SIZE;
    }

    // read the fifo content, FIFO_RW_CHUNK_SIZE bytes at a time
    len = 0;
    while (len < FILL_FIFO_SIZE) {
        res = inv_serial_read_fifo(mlsl_handle, g_mpu_slaveaddr, 
                                 FIFO_RW_CHUNK_SIZE, &read_data[len]);
        CHECK_RES;        
        len += FIFO_RW_CHUNK_SIZE;
    }

    // disable the fifo
    res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                MPUREG_USER_CTRL, usr_ctrl & ~BIT_FIFO_EN); 
    CHECK_RES;

    // compare
    for(j = 0; j < FILL_FIFO_SIZE; j++) {
        mismatches += (write_data[j] != read_data[j]);
    }
    if (mismatches) {
        if (g_verbose) {
            ADD_MESSAGE("\nwritten data:\n");
            print_data(write_data, FILL_FIFO_SIZE);
            ADD_MESSAGE("\nread data:\n");
            print_data(read_data, FILL_FIFO_SIZE);

            ADD_MESSAGE("\nMismatching bytes (%d):\n", mismatches);
            ADD_MESSAGE("\tloc. -> Wrote Read Bytes\n"); 
            for(j = 0; j < FILL_FIFO_SIZE; j++) {
                if (write_data[j] != read_data[j]) {
                    ADD_MESSAGE("\t%4d    0x%02X  0x%02X", 
                                j, write_data[j], read_data[j]);
                }
            }
            ADD_MESSAGE("\n");
        }
        RECORD_ERROR(mismatches, "FIFO fill failed on %d B out of %d", 
                                 mismatches, FILL_FIFO_SIZE);
    }

done:
    RECORD_ERROR(res, "FIFO fill failed - serial communication");
    RETURN_SUCCESS(r);
}

#define CHECK_RES_CONT if(res != INV_SUCCESS) {fail = true; continue;}

test_result test_bypass(void) 
{
    unsigned char data;
    unsigned char usr_ctrl;
    unsigned char sensors[12];
    unsigned short res;
    int fail = false;
    int bit_high_works = false;
    int bit_low_works = false;
    int i;

    RESULT_SETUP("Check bypass function"); r.res = 0;

    for(i = 0; i < 2; i++) {
        fail = 0;
        if(i == 0) {  // VDDIO level shifter off
            res = inv_serial_single_write(
                    mlsl_handle, g_mpu_slaveaddr, 
                    MPUREG_ACCEL_BURST_ADDR, g_accel_burstaddr ); 
            CHECK_RES_CONT;
        } else {     // VDDIO level shifter on
            res = inv_serial_single_write(
                    mlsl_handle, g_mpu_slaveaddr, 
                    MPUREG_ACCEL_BURST_ADDR, 0x80 | g_accel_burstaddr ); 
            CHECK_RES_CONT;
        }

        //turn on bypass mode
        res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                             MPUREG_USER_CTRL, 1, &usr_ctrl); 
        CHECK_RES_CONT;
        usr_ctrl &= ~BIT_AUX_IF_EN;        // clear this bit to enable by-pass
        res = inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, 
                                    MPUREG_USER_CTRL, usr_ctrl); 
        CHECK_RES_CONT;

        if(g_accel_slaveaddr == 0x0f) {  // Kionix accel
            res = inv_serial_read(mlsl_handle, g_accel_slaveaddr, 
                                 0x0f, 1, &data); 
            CHECK_RES_CONT;
            //RECORD_ERROR( 
            //    g_accel_slaveaddr==0x0f && data!=0x01, 
            //    "bypass mode check failed - reading Kionix WHO_AM_I yielt 0x%02X instead of 0x%02X", 
            //    data, 0x01
            //);
        }
        
        res = inv_serial_read(mlsl_handle, g_accel_slaveaddr, 
                             g_accel_burstaddr, g_accel_burstlen, sensors); 
        CHECK_RES_CONT;
        if (g_verbose) {
            ADD_MESSAGE(
                "VDDIO %3s - reg 0x00 : 0x%02X, accel[%02X:%02X] : "
                "0x%02X 0x%02x 0x%02X 0x%02x 0x%02X 0x%02x", 
                i==0?"OFF":"ON", data, 
                g_accel_burstaddr, g_accel_burstaddr + g_accel_burstlen,
                sensors[0], sensors[1], sensors[2], 
                sensors[3], sensors[4], sensors[5]
            );
        }

        if(i == 0 && !fail) {
            bit_low_works = true;
        }
        if(i == 1 && !fail){
            bit_high_works = true;
        }
    }

    usr_ctrl |= BIT_AUX_IF_EN;         // set bit to disable by-pass
    inv_serial_single_write(mlsl_handle, g_mpu_slaveaddr, MPUREG_USER_CTRL, usr_ctrl);
    inv_sleep(5);

    // NOTE : never reset MPU or secondary i2c while secondary i2c is enabled.

    RECORD_ERROR(fail, "Bypass mode check failed - serial communication");
    RECORD_ERROR_COND(bit_low_works == 0 && bit_high_works == 0, 
                      "Bypass mode check failed - neither VDDIO level works");

    if(bit_low_works) {
        ADD_MESSAGE("level shift bit low works");
    } else {
        ADD_MESSAGE("level shift bit low does not work, "
                    "ignore previous I2C error");
    }
    if(bit_high_works) {
        ADD_MESSAGE("level shift bit high works");
    } else {
        ADD_MESSAGE("level shift bit high does not work, "
                    "ignore previous I2C error");
    }

    RETURN_SUCCESS(r);
}



test_result test_gyro_sanity (void) 
{
    unsigned char sensors[3][6];
    unsigned short res;
    int i;
    int v1, v2, v3;

    RESULT_SETUP("Check gyro function");

    for(i=0; i< 3; i++) {
        res = inv_serial_read(mlsl_handle, g_mpu_slaveaddr, 
                             MPUREG_GYRO_XOUT_H, 6, sensors[i]);
        inv_sleep(10);
        CHECK_RES;
    }

    if (g_verbose) {
        for(i = 0; i < 3; i++) {
            ADD_MESSAGE(
                "sensors[%d] : 0x%02X 0x%02x 0x%02X 0x%02x 0x%02X 0x%02x", i, 
                sensors[i][0], sensors[i][1], sensors[i][2], 
                sensors[i][3], sensors[i][4], sensors[i][5]
            );
        }
    }

    for(i = 0; i < 3; i++) {
        v1 = sensors[0][i] * 256 + sensors[0][i + 1];
        v2 = sensors[1][i] * 256 + sensors[1][i + 1];
        v3 = sensors[2][i] * 256 + sensors[2][i + 1];

        if(v1 == v2 && v2 == v3) {
            RECORD_ERROR(1, "Gyro data sanity check failed - data unchanged");
        }
    }

done:
    RECORD_ERROR(res, "Gyro data sanity check failed - serial communication");
    RETURN_SUCCESS(r);
}



test_result test_accel_sanity(void) 
{
    RESULT_SETUP("Check accel data");
    RECORD_ERROR(INV_ERROR, "Accel test not implemlented");
    RETURN_SUCCESS(r);
}



/*
    MAIN 
*/

int main(int argc, const char *argv[])
{
    FILE* fp;
    test_result r;
    char outstr[250];
    unsigned char more = true;
    int i, j;

    // parse the command line to set up the global options
    parse_commandline(argc, argv);
#ifdef WIN32
    if(g_port == NULL) {
        printf("Error : no COM port number specified\n");
        printf("      : the test cannot continue\n");
        return(1);
    }
#endif
    sprintf(outstr, 
            "Test setup:\n"
            "\tPORT            %s\n"
            "\tWHOAMI          0x%02x\n"
            "\tMPU ADDR        0x%02x\n"
            "\tACCEL ADDR      0x%02x\n"
            "\tACCEL BURSTADDR 0x%02x\n"
            "\tACCEL BURSTLEN  0x%02x\n",
            g_port,
            g_whoami,
            g_mpu_slaveaddr, 
            g_accel_slaveaddr, 
            g_accel_burstaddr, 
            g_accel_burstlen
    );

    // open the test report file
    fp = fopen("test_report.txt", "w");
    if(fp == NULL) {
        puts("Could not open test report file.\n");
        exit(0);
    }

    OUTPUT(outstr, fp);

#ifdef LINUX_GPIO
    // configure GPIO
    gpio_setup(0,0,0); // GPIO0 output, initialvalue of 0;
    gpio_setup(1,0,0); // GPIO0 output, initialvalue of 0;
#endif

    while (more) {
        // loop over the array of tests, and run each one
        for(i = 0; i < sizeof(test_funcs) / sizeof(test_func); i++) {
            r = test_funcs[i]();
            OUTPUT(r.desc, fp);
            for(j=0; j<r.messages->num_msgs; j++) {
                sprintf(outstr, "\t%s", r.messages->msg[j]);
                OUTPUT(outstr, fp);
            }
            if(r.res) { 
                OUTPUT("\tPASSED.", fp); 
            } 
            else { 
                sprintf(outstr, "\tFAILED : %s", r.error);
                OUTPUT(outstr, fp);

#ifdef LINUX_GPIO
                gpio_set_value(1, 1);
                inv_sleep(1);
                gpio_set_value(1, 0);
#endif
                if (g_fail_on_error)
                    goto end;
            }
        }

        if (!g_continuous_run) {
            printf("\nDo you want to run the tests again? [Y/n] ");
            scanf("%s", outstr);
            if(outstr[0]=='n')
                more = false;
        }
    }

end:
    fclose(fp);

#ifdef _DEBUG
    printf("\nPress any key to exit... ");
    getchar();
#endif

    return 0;
}


/**
 *  @}
 */

