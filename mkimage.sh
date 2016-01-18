#!/bin/bash
set -e

. build/envsetup.sh >/dev/null && setpaths

export PATH=$ANDROID_BUILD_PATHS:$PATH
TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
TARGET_HARDWARE=`get_build_var TARGET_BOARD_HARDWARE`
echo TARGET_PRODUCT=$TARGET_PRODUCT
echo TARGET_HARDWARE=$TARGET_HARDWARE
TARGET="withoutkernel"
IMG_TARGET='all'
if [ $# -gt 0 ];then
        echo there are $# args : $@
        for tchip_arg in $@ 
                do 
                        [ $tchip_arg == 'boot' ] && IMG_TARGET=$tchip_arg
                        [ $tchip_arg == 'recovery' ] && IMG_TARGET=$tchip_arg
                        [ $tchip_arg == 'system' ] && IMG_TARGET=$tchip_arg
                        [ $tchip_arg == 'ota' ] && TARGET='ota'
                done
else
        IMG_TARGET='all'
        #TARGET='ota'
fi
echo IMG_TARGET=$IMG_TARGET , ota = $TARGET

IMAGE_PATH=rockdev/Image-$TARGET_PRODUCT

rm -rf $IMAGE_PATH
mkdir -p $IMAGE_PATH

FSTYPE=ext4
echo system filesysystem is $FSTYPE

BOARD_CONFIG=device/rockchip/common/device.mk

KERNEL_SRC_PATH=`grep TARGET_PREBUILT_KERNEL ${BOARD_CONFIG} |grep "^\s*TARGET_PREBUILT_KERNEL *:= *[\w]*\s" |awk  '{print $3}'`
KERNEL_SRC_PATH=`get_build_var TARGET_PREBUILT_KERNEL`

[ $(id -u) -eq 0 ] || FAKEROOT=fakeroot

BOOT_OTA="ota"

[ $TARGET != $BOOT_OTA -a $TARGET != "withoutkernel" ] && echo "unknow target[${TARGET}],exit!" && exit 0

    if [ ! -f $OUT/kernel ]
    then
	    echo "kernel image not fount![$OUT/kernel] "
        read -p "copy kernel from TARGET_PREBUILT_KERNEL[$KERNEL_SRC_PATH] (y/n) n to exit?"
        if [ "$REPLY" == "y" ]
        then
            [ -f $KERNEL_SRC_PATH ]  || \
                echo -n "fatal! TARGET_PREBUILT_KERNEL not eixit! " || \
                echo -n "check you configuration in [${BOARD_CONFIG}] " || exit 0

            cp ${KERNEL_SRC_PATH} $OUT/kernel

        else
            exit 0
        fi
    fi

cp -v ${KERNEL_SRC_PATH} $OUT/kernel

if [ $IMG_TARGET == 'boot' ] || [ $IMG_TARGET == 'all' ];then
if [ $TARGET == $BOOT_OTA ]
then
	echo "make ota images... "
	echo -n "create boot.img with kernel... "
	[ -d $OUT/root ] && \
	mkbootfs $OUT/root | minigzip > $OUT/ramdisk.img && \
        truncate -s "%4" $OUT/ramdisk.img && \
	mkbootimg --kernel $OUT/kernel --ramdisk $OUT/ramdisk.img --second kernel/resource.img --output $OUT/boot.img && \
	cp -a $OUT/boot.img $IMAGE_PATH/
	echo "done."
else
	echo -n "create boot.img without kernel... "
	[ -d $OUT/root ] && \
	mkbootfs $OUT/root | minigzip > $OUT/ramdisk.img && \
        truncate -s "%4" $OUT/ramdisk.img && \
	rkst/mkkrnlimg $OUT/ramdisk.img $IMAGE_PATH/boot.img >/dev/null
	echo "done."
fi
fi #### boot

if [ $IMG_TARGET == 'recovery' ] || [ $IMG_TARGET == 'all' ];then
if [ $TARGET == $BOOT_OTA ]
then
	echo -n "create recovery.img with kernel... "
	[ -d $OUT/recovery/root ] && \
	mkbootfs $OUT/recovery/root | minigzip > $OUT/ramdisk-recovery.img && \
        truncate -s "%4" $OUT/ramdisk-recovery.img && \
	mkbootimg --kernel $OUT/kernel --ramdisk $OUT/ramdisk-recovery.img --second kernel/resource.img  --output $OUT/recovery.img && \
	cp -a $OUT/recovery.img $IMAGE_PATH/
	echo "done."
else
	echo -n "create recovery.img with kernel and with out resource... "
	[ -d $OUT/recovery/root ] && \
	mkbootfs $OUT/recovery/root | minigzip > $OUT/ramdisk-recovery.img && \
        truncate -s "%4" $OUT/ramdisk-recovery.img && \
	mkbootimg --kernel $OUT/kernel --ramdisk $OUT/ramdisk-recovery.img --output $OUT/recovery.img && \
	cp -a $OUT/recovery.img $IMAGE_PATH/
	echo "done."
fi
fi #### recovery

        echo -n "create misc.img.... "
        cp -a rkst/Image/misc.img $IMAGE_PATH/misc.img
        cp -a rkst/Image/pcba_small_misc.img $IMAGE_PATH/pcba_small_misc.img
        cp -a rkst/Image/pcba_whole_misc.img $IMAGE_PATH/pcba_whole_misc.img
        echo "done."

if [ $IMG_TARGET == 'system' ] || [ $IMG_TARGET == 'all' ];then
if [ -d $OUT/system ]
then
	echo -n "create system.img... "
	if [ "$FSTYPE" = "cramfs" ]
	then
		chmod -R 777 $OUT/system
		$FAKEROOT mkfs.cramfs $OUT/system $IMAGE_PATH/system.img
	elif [ "$FSTYPE" = "squashfs" ]
	then
		chmod -R 777 $OUT/system
		mksquashfs $OUT/system $IMAGE_PATH/system.img -all-root >/dev/null
	elif [ "$FSTYPE" = "ext3" ] || [ "$FSTYPE" = "ext4" ]
	then
                system_size=`ls -l $OUT/system.img | awk '{print $5;}'`
                [ $system_size -gt "0" ] || { echo "Please make first!!!" && exit 1; }
                MAKE_EXT4FS_ARGS=" -L system -S $OUT/root/file_contexts -a system $IMAGE_PATH/system.img $OUT/system"
		ok=0
		while [ "$ok" = "0" ]; do
			make_ext4fs -l $system_size $MAKE_EXT4FS_ARGS >/dev/null 2>&1 &&
			tune2fs -c -1 -i 0 $IMAGE_PATH/system.img >/dev/null 2>&1 &&
			ok=1 || system_size=$(($system_size + 5242880))
		done
		e2fsck -fyD $IMAGE_PATH/system.img >/dev/null 2>&1 || true
	else
		mkdir -p $IMAGE_PATH/2k $IMAGE_PATH/4k
		mkyaffs2image -c 2032 -s 16 -f $OUT/system $IMAGE_PATH/2k/system.img
		mkyaffs2image -c 4080 -s 16 -f $OUT/system $IMAGE_PATH/4k/system.img
	fi
	echo "done."
fi
fi ##### system

cp kernel/kernel.img $IMAGE_PATH/
cp kernel/resource.img $IMAGE_PATH/
cp u-boot/RK3288UbootLoader*.bin $IMAGE_PATH/

echo "Copy kenrel.img..."
echo "Copy resource.img..."
cp kernel/kernel.img $IMAGE_PATH/
cp kernel/resource.img $IMAGE_PATH/

chmod a+r -R $IMAGE_PATH/
