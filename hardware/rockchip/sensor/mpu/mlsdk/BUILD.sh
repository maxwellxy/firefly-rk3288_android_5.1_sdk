#!/bin/bash

# This is a sample of the command line make used to build
#   the libraries and binaries.
# Please customize this path to match the location of your
#   Android source tree. Other variables may also need
#   to be customized such as:
#   $CROSS, $PRODUCT, $KERNEL_ROOT

#export ANDROID_BASE=/workspace/xmymk/rk_android4.1_in_sdk1.1
export ANDROID_BASE=/home/ywj/rk3188/android4.4

make MPU_NAME=MPU6050B1 VERBOSE=1 TARGET=android ANDROID_ROOT=${ANDROID_BASE} KERNEL_ROOT=${ANDROID_BASE}/kernel CROSS=${ANDROID_BASE}/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- PRODUCT=rk30sdk MPL_LIB_NAME=mplmpu echo_in_colors=echo -f Android-shared.mk

make MPU_NAME=MPU6050B1 VERBOSE=1 TARGET=android ANDROID_ROOT=${ANDROID_BASE} KERNEL_ROOT=${ANDROID_BASE}/kernel CROSS=${ANDROID_BASE}/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- PRODUCT=rk30sdk MPL_LIB_NAME=mplmpu echo_in_colors=echo -f Android-static.mk


