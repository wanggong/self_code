adb remount
adb shell chmod 777 /system/bin/
adb push i2c_rw /system/bin/
adb shell chmod 777 /system/bin/i2c_rw
adb shell chmod 777 /system/lib/modules/
adb push debug_mem.ko /system/lib/modules/
adb shell rmmod debug_mem
adb shell insmod /system/lib/modules/debug_mem.ko
adb shell mkdir -p /data/debugfs/
adb shell mount -t debugfs nodev /data/debugfs
adb push gpio.sh /system/bin/
adb shell chmod 777 /system/bin/gpio.sh
adb push csid.sh /system/bin/
adb shell chmod 777 /system/bin/csid.sh
adb push pmic.sh /system/bin/
adb shell chmod 777 /system/bin/pmic.sh



