MAKE_CMD="make \
    VERBOSE=0 \
    TARGET=android \
    CROSS=/mnt/cdy/tureqi_4.2.2_r1/prebuilts/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- \
    ANDROID_ROOT=/mnt/cdy/tureqi_4.2.2_r1 \
    KERNEL_ROOT=/mnt/cdy/tureqi_4.2.2_r1 \
    PRODUCT=rk30sdk \
    DEVICE=MPU6050b1 \
    MPL_LIB_NAME=mplmpu \
"

#$MAKE_CMD -f Android-static.mk
$MAKE_CMD -f Android-static.mk clean
#$MAKE_CMD -f Android-shared.mk
$MAKE_CMD -f Android-shared.mk clean
