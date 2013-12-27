function while_function()
{
	index=0;
	while [ 1 ]
	do
	am start -a android.media.action.IMAGE_CAPTURE
	sleep 10
	input keyevent 3
	sleep 10
	done
}
logcat -v threadtime > /data/logcat.log&
cat /proc/kmsg > /data/kmsg.log&
while_function