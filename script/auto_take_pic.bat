adb shell input keyevent 27
sleep 2
adb shell mv /sdcard/DCIM/Camera/*.jpg /sdcard/DCIM/Camera/test.jpg
adb pull /sdcard/DCIM/Camera/test.jpg takepic/%1.jpg
adb shell rm /sdcard/DCIM/Camera/*
