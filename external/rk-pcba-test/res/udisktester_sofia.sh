#!/system/bin/sh

#RESULT_FILE="/data/udisk_capacity.txt"
#LOG_FILE="/data/udisk.log"
#source send_cmd_pipe.sh

result_file_udisk=/data/udisk_capacity.txt

if [ -e $result_file2 ] ; then
busybox rm -f $result_file_udisk
fi

#while true; do
    for nr in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
        udisk="/dev/block/sd$nr"
        udiskp=$udisk"1"
        part=$udisk
    
        echo "searching disk ..."
        while true; do
            while true; do
                if [ -b "$udisk" ]; then
                    busybox sleep 1
                    if [ -b "$udisk" ]; then
                        echo "udisk insert"
                        #add by wjh
                        echo "ok" > /data/udisk_capacity.txt
                        exit 1
                        #break;
                    fi
                else
                    busybox sleep 1
                fi
            done
            
            if [ ! -d "/mnt/usb_storage" ]; then
                busybox mkdir -p /mnt/usb_storage
            fi
            
            echo "mounting disk ..."
            mount -t vfat $udiskp /mnt/usb_storage
            if [ $? -ne 0 ]; then
                mount -t vfat $udisk /mnt/usb_storage
                if [ $? -ne 0 ]; then
                    echo "udisk mount failed"
                    exit 1
                    #SEND_CMD_PIPE_FAIL $3
                    #busybox sleep 3
                    # goto for nr in ...
                    # detect next plugin, the devno will changed
                    #continue 2
                else
                    part=$udiskp
                fi
            fi
    
            break
        done
    
        capacity=`df | grep "/mnt/usb_storage" | busybox awk '{printf $2}'`
        echo "udisk $part: $capacity"

        umount /mnt/usb_storage
        #SEND_CMD_PIPE_OK_EX $3 $capacity

        echo $capacity > /data/udisk_capacity.txt
        break
    
#        while true; do
#            if [ -b "$udisk" ]; then
#                echo "please remove udisk"
#                busybox sleep 1
#            else
#                echo "udisk removed"
#                break
#            fi
#        done
    done
#done

