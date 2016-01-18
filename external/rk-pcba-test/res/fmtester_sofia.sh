#!/system/bin/sh

#result_file=/data/fm_info

#if [ -e $result_file ] ; then
#busybox rm -f $result_file
#fi

#busybox touch /data/fm_info

#busybox chmod 777 /data/fm_info

echo "" > /data/fm_info

at_cli_client "at@audapp:fmr_disable()"

at_cli_client "at@audapp:fmrx_enable()"

at_cli_client "at@audapp:fmrx_set_station(5,0,0,0)"

at_cli_client "at@audapp:fmrx_get_auto_seek_report()" outfile=/data/fm_info

#gpgsv=`cat /data/fm_info | busybox cut -d " " -f 1 | busybox sed -n '4p'`

#echo $gpgsv > /data/fm_channel_counts

#gpgsv=`cat /data/fm_info | busybox cut -d { -f 2 | busybox cut -d , -f 1 | busybox sed -n '5p'`

#echo $gpgsv > /data/fm_info

cat /data/fm_info
exit 1

