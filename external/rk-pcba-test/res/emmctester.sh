#!/sbin/sh

#source send_cmd_pipe.sh

nr="1"
mmcblk="/dev/block/mmcblk$nr"
mmcp=$mmcblk

#while true; do
    while true; do
        while true; do
            if [ -b "$mmcblk" ]; then
              busybox  sleep 1
                if [ -b "$mmcblk" ]; then
                    echo "card$nr insert"
                    break
                fi
            else
               busybox sleep 1
            fi
        done
        
        if [ ! -d "/tmp/extsd" ]; then
            busybox mkdir -p /tmp/extsd
        fi
        
        mmcp=$mmcblk
		echo $mmcp
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

        break
    done
    
    capacity=`busybox df | busybox grep "/tmp/extsd" | busybox awk '{printf $2}'`
    echo "$mmcp: $capacity"
    
    busybox umount /tmp/extsd
    
    echo $capacity > /data/sd_capacity

	exit 1
#    while true; do
 #       if [ -b "$mmcblk" ]; then
#            echo "please remove card$nr"
#            busybox sleep 1
 #       else
  #          echo "card$nr remove"
  #          break
   #     fi
    #done
#done

