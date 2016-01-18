#!/bin/bash

. build/envsetup.sh >/dev/null && setpaths

ANDROID_ROOT="$(get_abs_build_var)"
KERNEL_ROOT="$ANDROID_ROOT/kernel"
TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
IMG_ROOT="$ANDROID_ROOT/rockdev/Image-$TARGET_PRODUCT"
IMG_LIST="kernel.img  resource.img  boot.img  misc.img  pcba_small_misc.img  pcba_whole_misc.img  recovery.img  system.img"
SRC_RAR_FILE="$ANDROID_ROOT/FFTools/AndroidTool.rar"
DST_RAR_PATH="$ANDROID_ROOT/rockdev/Image-$TARGET_PRODUCT/"

cp $KERNEL_ROOT/kernel.img $DST_RAR_PATH
cp $KERNEL_ROOT/resource.img $DST_RAR_PATH

if [ -d $IMG_ROOT ];then
    if [ ! -e $SRC_RAR_FILE ];then
	
        echo "Make sure you have file \"$SRC_RAR_FILE\"!"
        exit 2
    fi        
    for img in $IMG_LIST
    do
        if [ ! -e $IMG_ROOT/$img ];then
            echo "Make sure you have file \"$img\"!"
            exit 3
        fi
    done
else
    echo "Make sure you have directory \"$IMG_ROOT\"!"
    exit 1
fi

which rar > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Make sure you have tool \"rar\""
    exit 4
fi

if [ "z${1}" != "z" ] ; then
	DST_RAR_NAME="${1}.rar"
else
	DST_RAR_NAME="FireFly_Android$(get_build_var PLATFORM_VERSION)_$(get_build_var TARGET_BUILD_VARIANT)_$(date -d today +%y%m%d_%k%M).rar"
fi
DST_RAR_NAME=$(echo $DST_RAR_NAME | sed s/[[:space:]]//g)

cd $IMG_ROOT
cp "$SRC_RAR_FILE" "$DST_RAR_NAME"

# put all the *img and update log into rockdev/Image/
rar a -ap"rockdev/Image/" $DST_RAR_NAME $IMG_LIST

