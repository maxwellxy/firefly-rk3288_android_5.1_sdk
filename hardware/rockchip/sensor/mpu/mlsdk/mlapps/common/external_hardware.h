/*
    Accelerometer
*/
#define get_accel_slave_descr NULL

#ifdef CONFIG_MPU_SENSORS_ADXL34X	/* ADI accelerometer */
struct ext_slave_descr *adxl34x_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr adxl34x_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_BMA150	/* Bosch accelerometer */
struct ext_slave_descr *bma150_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr bma150_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_BMA222	/* Bosch 222 accelerometer */
struct ext_slave_descr *bma222_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr bma222_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_BMA250	/* Bosch accelerometer */
struct ext_slave_descr *bma250_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr bma250_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_KXSD9	/* Kionix accelerometer */
struct ext_slave_descr *kxsd9_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr kxsd9_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_KXTF9	/* Kionix accelerometer */
struct ext_slave_descr *kxtf9_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr kxtf9_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_LIS331DLH	/* ST accelerometer */
struct ext_slave_descr *lis331_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr lis331_get_slave_descr
#endif


#ifdef CONFIG_MPU_SENSORS_LIS3DH	/* ST accelerometer */
struct ext_slave_descr *lis3dh_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr lis3dh_get_slave_descr
#endif

/* ST accelerometer in LSM303DLx combo */
#if defined CONFIG_MPU_SENSORS_LSM303DLX_A
struct ext_slave_descr *lsm303dlx_a_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr lsm303dlx_a_get_slave_descr
#endif

/* MPU6050 Accel */
struct ext_slave_descr *mpu6050_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr mpu6050_get_slave_descr

#ifdef CONFIG_MPU_SENSORS_MMA8450	/* Freescale accelerometer */
struct ext_slave_descr *mma8450_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr mma8450_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_MMA845X	/* Freescale accelerometer */
struct ext_slave_descr *mma845x_get_slave_descr(void);
#undef get_accel_slave_descr
#define get_accel_slave_descr mma845x_get_slave_descr
#endif


/*
    Compass
*/
#define get_compass_slave_descr NULL

#ifdef CONFIG_MPU_SENSORS_AK8975	/* AKM compass */
struct ext_slave_descr *ak8975_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr ak8975_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_AMI30X	/* AICHI Steel  AMI304/305 compass */
struct ext_slave_descr *ami30x_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr ami30x_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_AMI306	/* AICHI Steel AMI306 compass */
struct ext_slave_descr *ami306_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr ami306_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_HMC5883	/* Honeywell compass */
struct ext_slave_descr *hmc5883_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr hmc5883_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_MMC314X	/* MEMSIC compass */
struct ext_slave_descr *mmc314x_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr mmc314x_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_LSM303DLX_M	/* ST compass */
struct ext_slave_descr *lsm303dlx_m_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr lsm303dlx_m_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_YAS529	/* Yamaha compass */
struct ext_slave_descr *yas529_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr yas529_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_YAS530	/* Yamaha compass */
struct ext_slave_descr *yas530_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr yas530_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_HSCDTD002B	/* Alps HSCDTD002B compass */
struct ext_slave_descr *hscdtd002b_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr hscdtd002b_get_slave_descr
#endif

#ifdef CONFIG_MPU_SENSORS_HSCDTD004A	/* Alps HSCDTD004A compass */
struct ext_slave_descr *hscdtd004a_get_slave_descr(void);
#undef get_compass_slave_descr
#define get_compass_slave_descr hscdtd004a_get_slave_descr
#endif
/*
    Pressure
*/
#define get_pressure_slave_descr NULL

#ifdef CONFIG_MPU_SENSORS_BMA085	/* BMA pressure */
struct ext_slave_descr *bma085_get_slave_descr(void);
#undef get_pressure_slave_descr
#define get_pressure_slave_descr bma085_get_slave_descr
#endif
