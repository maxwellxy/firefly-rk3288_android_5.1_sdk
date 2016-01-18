#!/system/bin/sh

mount -o rw,remount -t ext3 /dev/block/mtdblock8 /system 

#sleep 2

echo "*******************************"
echo     "wh  do begin"
echo "*******************************"
rm system/app/*.nm
