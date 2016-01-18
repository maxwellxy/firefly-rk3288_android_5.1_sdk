#!/system/bin/sh
log -t PackageManager "Start to clean up /system/preinstall_del/"
mount -o rw,remount -t ext4 /system
rm system/preinstall_del/*.*
mount -o ro,remount -t ext4 /system
