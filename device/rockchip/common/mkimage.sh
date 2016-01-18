#!/bin/bash
set -e

. build/envsetup.sh >/dev/null && setpaths

export PATH=$ANDROID_BUILD_PATHS:$PATH
TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
TARGET_HARDWARE=`get_build_var TARGET_BOARD_HARDWARE`
echo TARGET_PRODUCT=$TARGET_PRODUCT
echo TARGET_HARDWARE=$TARGET_HARDWARE
TARGET="withoutkernel"
if [ "$1"x != ""x  ]; then
         TARGET=$1
fi

IMAGE_PATH=rockdev/Image-$TARGET_PRODUCT

rm -rf $IMAGE_PATH
mkdir -p $IMAGE_PATH

FSTYPE=ext4
echo system filesysystem is $FSTYPE

BOARD_CONFIG=device/rockchip/common/device.mk

KERNEL_SRC_PATH=`grep TARGET_PREBUILT_KERNEL ${BOARD_CONFIG} |grep "^\s*TARGET_PREBUILT_KERNEL *:= *[\w]*\s" |awk  '{print $3}'`

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
	echo -n "create misc.img.... "
	cp -a rkst/Image/misc.img $IMAGE_PATH/misc.img
	cp -a rkst/Image/pcba_small_misc.img $IMAGE_PATH/pcba_small_misc.img
	cp -a rkst/Image/pcba_whole_misc.img $IMAGE_PATH/pcba_whole_misc.img
	echo "done."

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

chmod a+r -R $IMAGE_PATH/
