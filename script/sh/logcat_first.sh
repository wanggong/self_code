#!/system/bin/sh
file_name=`date +%Y-%m-%d-%H-%M-%S`
mkdir  /data/logs/;
cat /proc/kmsg > /data/logs/$file_name"_kmsg.log"&
logcat -v threadtime -r 50000 -n 2000 -f  /data/logs/$file_name"logcat.log"


