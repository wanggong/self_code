adb remount
adb shell chmod 777 /system/bin/
adb shell mkdir -p /data/debugfs/
adb shell mount -t debugfs nodev /data/debugfs
adb push sp2529.sh /system/bin/
adb shell chmod 777 /system/bin/sp2529.sh 
adb shell sp2529.sh 