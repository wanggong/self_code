adb shell tigerwolf -c mount -o remount rw /system/
adb shell tigerwolf -c chmod 777 /system/bin/
adb push i2c_rw /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/i2c_rw
adb shell tigerwolf -c chmod 777 /system/lib/modules/
adb push debug_mem.ko /system/lib/modules/
adb shell tigerwolf -c rmmod debug_mem
adb shell tigerwolf -c insmod /system/lib/modules/debug_mem.ko
adb shell tigerwolf -c mkdir -p /data/debugfs/
adb shell tigerwolf -c mount -t debugfs nodev /data/debugfs
adb push gpio.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/gpio.sh
adb push csid.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/csid.sh
adb push dsi.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/dsi.sh
adb push pmic.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/pmic.sh
adb push sp2529.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/sp2529.sh
adb push base.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/base.sh
adb push suspend_resume.sh /system/bin/
adb shell tigerwolf -c chmod 777 /system/bin/suspend_resume.sh




