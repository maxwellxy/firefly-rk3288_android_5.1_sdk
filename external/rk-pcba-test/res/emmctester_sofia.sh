#!/system/bin/busybox sh

#source send_cmd_pipe.sh

mmcblk="/dev/block/mmcblk1p1"
mmcp=$mmcblk

if [ ! -b "$mmcblk" ]; then
	echo "not card insert"
	exit 0
fi        

if [ ! -d "/tmp/extsd" ]; then
    busybox mkdir -p /tmp/extsd
fi

mmcp=$mmcblk
busybox mount -t vfat $mmcp /tmp/extsd
if [ $? -ne 0 ]; then
    mmcp=$mmcblk"p1"
   busybox mount -t vfat $mmcp /tmp/extsd
    if [ $? -ne 0 ]; then
        exit 0
        busybox sleep 3
        continue 2
    fi
fi

capacity=`busybox df | busybox grep "/tmp/extsd" | busybox awk '{printf $2}'`
echo "$mmcp: $capacity"

busybox umount /tmp/extsd
echo $capacity > /data/sd_capacity
exit 1

