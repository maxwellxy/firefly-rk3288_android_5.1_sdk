#!/system/bin/sh

# 在init.方案.rc下加入如下语句
# service  catlog /system/bin/busybox  sh  /system/bin/cat_log.sh
#     disabled
#     oneshot
#
# on property:sys.boot_completed=1
#   start catlog
#

echo 1 > /proc/sys/kernel/panic

#命名文件夹名字e5_log,也可以自己更改
ENABLE_LOG_FILE="/mnt/sdcard/.enable_logsave"
LOG_DIR="/mnt/sdcard/.LOGSAVE"
LAST_LOG_DIR="/mnt/sdcard/.LOGSAVE/last"
SAVE_LOG_COUNT=5   # 保存上5次的log，值最小为1;例为5,则last.1为最后一次重启前的log；last.5为最老的log

#echo “save last_time  ${SAVE_LOG_COUNT} log”


#if [ ! -f "$ENABLE_LOG_FILE" ];then                     
#   echo "disable logsave"                    
#   exit                                                 
                                                        
#fi 

#echo "enable logsave"

if [ ! -d "$LOG_DIR" ];then
   mkdir $LOG_DIR
fi

if [  -d "$LAST_LOG_DIR.$SAVE_LOG_COUNT" ];then
   rm -r "$LAST_LOG_DIR.$SAVE_LOG_COUNT"
fi


#for((i= ${SAVE_LOG_COUNT}-1; $i >= 1 ;i--));do
#for i in  $(seq  `expr $SAVE_LOG_COUNT - 1`  -1 1)
i=$((SAVE_LOG_COUNT -1))
while [ $i -ge 1 ]
do 
        if [  -d "$LAST_LOG_DIR.$i" ];then
               #echo "$LAST_LOG_DIR.$i is exists "
               if [ "`ls -a $LAST_LOG_DIR.$i`" = "" ]; then
                       echo "$LAST_LOG_DIR.$i is indeed empty"
               else
                       echo "$LAST_LOG_DIR.$i is not empty"
                       #j=`expr $i + 1`
                       j=$(($i+1)) 
                       mv  "$LAST_LOG_DIR.$i"  "$LAST_LOG_DIR.$j"
               fi

               
       #else
               #echo "$LAST_LOG_DIR.$i isnot exists"
               
        fi
        i=$(($i-1))
         
done

#创建上一次日志保存目录
mkdir $LAST_LOG_DIR."1"

#保存上次开机之后的log
mv $LOG_DIR/*.log $LAST_LOG_DIR."1"
mv $LOG_DIR/*.log* $LAST_LOG_DIR."1"


DATE=$(date +%Y%m%d%H%M)

cat /proc/last_kmsg > $LOG_DIR/"$DATE"_panic_kmsg.log

echo "------start kmsg log------"
cat /proc/kmsg > $LOG_DIR/"$DATE"_kmsg.log &

echo "------start logcat log------"
logcat -v time -n 1 -f $LOG_DIR/"$DATE"_logcat.log -r10240 

