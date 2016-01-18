#!/bin/bash
echo res.sh $1 $2 $3
TARGET_PRODUCT=$1
PRODUCT_OUT=$2
TARGET_BOARD_PLATFORM=$3
TARGET_COMMON=common
PCBA_PATH=external/rk-pcba-test
BT_BLUEDROID=true
if [ $TARGET_BOARD_PLATFORM = "rk2928" ]; then
    MODULE="modules"
else
    MODULE="modules_smp"
fi
echo MODULE $MODULE

############################################### wifi bt firmware ##################################################

mkdir -p $PRODUCT_OUT/recovery/root/system/
mkdir -p $PRODUCT_OUT/recovery/root/system/bin/
mkdir -p $PRODUCT_OUT/recovery/root/system/lib/
mkdir -p $PRODUCT_OUT/recovery/root/system/lib/modules/
mkdir -p $PRODUCT_OUT/recovery/root/system/lib/hw/
mkdir -p $PRODUCT_OUT/recovery/root/system/etc/
mkdir -p $PRODUCT_OUT/recovery/root/system/etc/firmware/
mkdir -p $PRODUCT_OUT/recovery/root/etc/firmware/
mkdir -p $PRODUCT_OUT/recovery/root/vendor/firmware
mkdir -p $PRODUCT_OUT/recovery/root/lib/firmware
mkdir -p $PRODUCT_OUT/recovery/root/system/etc/bluetooth
if [ -e "$PRODUCT_OUT/system/etc/firmware/" ] ; then
cp $PRODUCT_OUT/system/etc/firmware/ $PRODUCT_OUT/recovery/root/system/etc/ -a
fi
if [ -e "$PRODUCT_OUT/system/vendor/firmware/" ] ; then
cp $PRODUCT_OUT/system/vendor/firmware/ $PRODUCT_OUT/recovery/root/vendor/ -a
fi

if [ -e "$PRODUCT_OUT/system/etc/firmware/rtl8723b_fw" ] ; then
cp $PRODUCT_OUT/system/etc/firmware/rtl8723b_fw $PRODUCT_OUT/recovery/root/lib/firmware/rtl8723b_fw
cp $PRODUCT_OUT/system/etc/firmware/rtl8723bu_config $PRODUCT_OUT/recovery/root/lib/firmware/rtl8723bu_config
cp $PRODUCT_OUT/system/etc/firmware/rtl8723a_fw $PRODUCT_OUT/recovery/root/lib/firmware/rtl8723a_fw
cp $PRODUCT_OUT/system/etc/firmware/rtl8723a_config $PRODUCT_OUT/recovery/root/lib/firmware/rtl8723a_config
fi
############################################### ko ##################################################

if [ -e "device/rockchip/$TARGET_COMMON/ipp/lib/rk29-ipp.ko" ] ; then
cp device/rockchip/$TARGET_COMMON/ipp/lib/rk29-ipp.ko $PRODUCT_OUT/recovery/root/
fi
if [ -e "device/rockchip/$TARGET_COMMON/ipp/lib/rk29-ipp.ko.3.0.36+" ] ; then
cp device/rockchip/$TARGET_COMMON/ipp/lib/rk29-ipp.ko.3.0.36+ $PRODUCT_OUT/recovery/root/
fi

############################################### bin/lib ##################################################

cp -rf $PCBA_PATH/sbin/* $PRODUCT_OUT/recovery/root/system/bin/
cp -rf $PCBA_PATH/sbin/* $PRODUCT_OUT/recovery/root/sbin/

if [ -e "$PRODUCT_OUT/system/bin/toolbox" ] ; then
cp $PRODUCT_OUT/system/bin/toolbox $PRODUCT_OUT/recovery/root/system/bin/
fi
if [ -e "$PRODUCT_OUT/system/bin/linker" ] ; then
cp $PRODUCT_OUT/system/bin/linker $PRODUCT_OUT/recovery/root/system/bin/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libselinux.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libselinux.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libusbhost.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libusbhost.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libc.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libc.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libcutils.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libcutils.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/liblog.so" ] ; then
cp $PRODUCT_OUT/obj/lib/liblog.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libm.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libm.so $PRODUCT_OUT/recovery/root/system/lib/
fi
if [ -e "$PRODUCT_OUT/obj/lib/libstdc++.so" ] ; then
cp $PRODUCT_OUT/obj/lib/libstdc++.so $PRODUCT_OUT/recovery/root/system/lib/
fi

if [ $BT_BLUEDROID = "true" ] ; then
    if [ -e "$PRODUCT_OUT/obj/lib/bluetooth.default.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/bluetooth.default.so $PRODUCT_OUT/recovery/root/system/lib/hw/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbluetooth_mtk.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbluetooth_mtk.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbt-hci.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbt-hci.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbt-utils.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbt-utils.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/vendor/lib/libbt-vendor.so" ] ; then
    cp $PRODUCT_OUT/obj/vendor/lib/libbt-vendor.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbt-vendor.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbt-vendor.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbt-vendor-rtl8723bu.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbt-vendor-rtl8723bu.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbt-vendor-rtl8723bu.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbt-vendor-rtl8723bu.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libcorkscrew.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libcorkscrew.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libgccdemangle.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libgccdemangle.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libhardware_legacy.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libhardware_legacy.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libhardware.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libhardware.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libnetutils.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libnetutils.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libpower.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libpower.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libutils.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libutils.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libwpa_client.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libwpa_client.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libz.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libz.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libbacktrace.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libbacktrace.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libstlport.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libstlport.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libunwind.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libunwind.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libcrypto.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libcrypto.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libdl.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libdl.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/obj/lib/libunwind-ptrace.so" ] ; then
    cp $PRODUCT_OUT/obj/lib/libunwind-ptrace.so $PRODUCT_OUT/recovery/root/system/lib/
    fi
    if [ -e "$PRODUCT_OUT/system/bin/bdt" ] ; then
    cp $PRODUCT_OUT/system/bin/bdt $PRODUCT_OUT/recovery/root/system/bin/
    fi 
    if [ -e "$PRODUCT_OUT/system/etc/bluetooth/" ] ; then
    cp $PRODUCT_OUT/system/etc/bluetooth/ $PRODUCT_OUT/recovery/root/system/etc/ -a
    fi
fi
