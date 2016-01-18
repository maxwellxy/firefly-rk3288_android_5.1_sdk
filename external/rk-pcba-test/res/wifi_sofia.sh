#!/system/bin/busybox sh

wpa_supplicant_file=/data/misc/wifi/wpa_supplicant.conf
result_file=/data/scan_result.txt
result_file2=/data/scan_result2.txt
result_file3=/data/scan_result3.txt
result_file4=/data/scan_result4.txt
result_file5=/data/scan_result5.txt
result_file6=/data/scan_result6.txt
result_file7=/data/scan_result7.txt
nr=1

if [ -e $result_file ] ; then
busybox rm -f $result_file
fi

if [ -e $result_file2 ] ; then
busybox rm -f $result_file2
fi

rmmod iwlxvt
rmmod iwlmvm
rmmod iwlwifi

insmod /system/lib/modules/iwlwifi.ko nvm_file=nvmDataDefault
insmod /system/lib/modules/iwlmvm.ko

echo "wlan test sleep 1s"
busybox sleep 1

busybox ifconfig wlan0 up

busybox cp /etc/wifi/wpa_supplicant.conf /data/misc/wifi/wpa_supplicant.conf
busybox chmod 0777 /data/misc/wifi/wpa_supplicant.conf

start wpa_supplicant

echo "sleep 1s"
busybox sleep 1

while true; do
	echo "" > $result_file4
	
	wpa_cli ifname=wlan0 scan

	#echo "sleep 1s"
	busybox sleep 1

	wpa_cli ifname=wlan0 scan_results > $result_file4
	echo "wjh000000000000000000000000"
	#busybox sed -n '$=' $result_file4
	[ "`busybox wc -l < $result_file4`" -gt 2 ] && break

	if [ $nr -eq 5 ]; then
		exit 1    
    fi
    nr=`busybox expr $nr + 1`
    busybox sleep 1
done

if [ $(busybox sed -n '$=' $result_file4) -gt $bb ]; then
	busybox tail -1 $result_file4 > $result_file3
	busybox tail -1 $result_file4 > $result_file7
	cat $result_file3 | busybox awk '{print $5}' > $result_file5
	busybox awk '{printf $0; if (getline) print}' $result_file5 > $result_file
	
	cat $result_file7 | busybox awk '{print $3}' > $result_file6
	busybox awk '{printf $0; if (getline) print}' $result_file6 > $result_file2
	busybox rm -f $result_file3
	busybox rm -f $result_file5
	busybox rm -f $result_file6
	busybox rm -f $result_file7
fi

busybox rm -f $result_file4

if [ -e $wpa_supplicant_file ]; then
	busybox rm -f $wpa_supplicant_file
fi

exit 1

