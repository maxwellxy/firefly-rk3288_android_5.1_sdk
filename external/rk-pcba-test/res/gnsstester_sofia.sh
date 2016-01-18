#!/system/bin/sh

result_file=/data/gnss_info

if [ -e $result_file ] ; then
busybox rm -f $result_file
fi

at_cli_client "at@gnss:get_gnss_host_sw_version()"

at_cli_client "at@gnss:clear_assist_data(65535)"

at_cli_client "at@gnss:store_lcs_qos(100,100,20856,1000)"

at_cli_client "at@gnss:position_fix_request(2,0,3,511,2)"

busybox sleep 22
gpgsv=`cat /data/gnss/LBS_NMEA.txt | grep "GPGSV"`

busybox echo $gpgsv > /data/gnss_info
#busybox rm -f /data/gnss/LBS_NMEA.txt
exit 1

