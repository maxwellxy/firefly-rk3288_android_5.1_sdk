#!/system/bin/sh

result_file2=/data/sd_capacity
nand_file=/proc/nand

if [ -e $result_file2 ] ; then
busybox rm -f $result_file2
fi

while true; do

	nr=1
	if [ -e $nand_file ] ; then
		nr=0
	fi
	mmcblk="/dev/block/mmcblk$nr"
	#echo $mmcblk
	mmcp=$mmcblk
	
    while true; do
        while true; do
        	#echo $mmcblk
            if [ -b "$mmcblk" ]; then
                #busybox  sleep 1
                if [ -b "$mmcblk" ]; then
                    echo "card$nr insert"
                    #break
                    echo "ok" > /data/sd_capacity
                    exit 1
                fi
            else
            	if [ $nr -eq 100 ]; then
			       #echo "PCBA TEST SDcard:mmcblk1-->100 can't find card..."
			       busybox sleep 2
			       break 2
			    fi
			    nr=`busybox expr $nr + 1`
				mmcblk="/dev/block/mmcblk$nr"
				#echo $mmcblk
				continue 2
            fi
        done
        
        if [ ! -d "/mnt/external_sd" ]; then
            busybox mkdir -p /mnt/external_sd
        fi

		umount /mnt/external_sd
        mmcp=$mmcblk"p1"
        #mmcp=$mmcblk
        echo $mmcp
        mount -t vfat $mmcp /mnt/external_sd
        if [ $? -ne 0 ]; then
        	echo "mount error."
			if [ $nr -eq 100 ]; then
			       echo "PCBA TEST SDcard:mmcblk1-->100 can't mount card..."
			       busybox sleep 2
			       break
		    fi
		    nr=`busybox expr $nr + 1`
			mmcblk="/dev/block/mmcblk$nr"
			#echo $mmcblk
			continue 1

            mmcp=$mmcblk"p1"
            echo $mmcp
            mount -t vfat $mmcp /mnt/external_sd
            if [ $? -ne 0 ]; then
            	echo "mount error."
            fi
        fi

        break 2
    done
done
	echo "PCBA TEST SDcard:$mmcp: mount success."
    capacity=`df | grep "/mnt/external_sd" | busybox awk '{printf $2}'`
    echo "$mmcp: $capacity"
    
    umount /mnt/external_sd
    
    echo $capacity > /data/sd_capacity

	exit 1

