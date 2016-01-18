#!/bin/bash

source ../../../build/envsetup.sh

croot

echo "---- make installclean ----"
make installclean

echo "---- mmm ----"
mmm external/bluetooth/bluedroid/ -B -j4
mmm external/bluetooth_bplus/bluedroid/ -B -j4
mmm external/broadcom/ -B -j4
mmm device/common/bluetooth/libbt/ -B -j4
mmm device/common/bluetooth/libbt_beken/ -B -j4
mmm device/common/bluetooth/libbt_rda/ -B -j4
mmm device/common/bluetooth/libbt_rtk/ -B -j4
mmm device/common/bluetooth/libbt_rtk8723bs/ -B -j4
mmm device/common/bluetooth/libbt_rtk_usb/ -B -j4
mmm hardware/libhardware_legacy/ -B -j4
mmm hardware/realtek/wlan/ -B -j4
mmm hardware/mediatek/ -B -j4
mmm hardware/broadcom/wlan/bcmdhd/ -B -j4
mmm hardware/esp/wlan/ -B -j4
mmm system/netd/ -B -j4
#mmm frameworks/base/core/jni/ -B -j4
#mmm frameworks/base/core/res -B -j4
#mmm frameworks/base/ -B -j4
mmm packages/apps/Bluetooth -B -j4
mmm packages/apps/Bluetooth_bplus/ -B -j4

echo "---- make -j4 ----"
make -j4

mmm external/wpa_supplicant_8/ -B -j4
