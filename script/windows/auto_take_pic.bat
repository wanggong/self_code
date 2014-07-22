adb shell input keyevent 27
rem sleep 2
ping localhost > nul
adb shell mv /sdcard/DCIM/Camera/*.jpg /sdcard/DCIM/Camera/test.jpg
adb shell mv /sdcard/DCIM/Camera/raw/*.raw /sdcard/DCIM/Camera/raw/test.raw
adb pull /sdcard/DCIM/Camera/test.jpg takepic/%1.jpg
adb pull /sdcard/DCIM/Camera/raw/test.raw takepic/raw/%1.raw
adb shell rm -fr /sdcard/DCIM/Camera/test.jpg
adb shell rm -fr /sdcard/DCIM/Camera/raw/test.raw
