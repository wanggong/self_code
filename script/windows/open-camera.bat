:: adb shell am start -n com.android.camera/com.android.camera.Camera
:wangqin
adb shell am start -a android.media.action.IMAGE_CAPTURE
sleep 10
adb shell input keyevent 3
sleep 10
goto wangqin