file_name=`date +%Y-%m-%d-%H-%M-%S`
setenforce 0;
mkdir  /data/logs/;
cat /proc/kmsg > /data/logs/$file_name"_kmsg.log"&
#sleep 3;
logcat -v threadtime -r 50000 -n 10 -f  /data/logs/$file_name"logcat.log"&


mkdir /data/logs/
setprop persist.camera.sensor.debug 3
cat /sys/module/smb358_charger/parameters/disable_software_temp_monitor
#cat /proc/kmsg > /data/logs/kmsg.log&
#logcat -v threadtime -r 50000 -n 1000 -f  /data/logs/logcat.log&
monkey -p org.codeaurora.snapcam -v -v -v --ignore-crashes --monitor-native-crashes --ignore-security-exceptions --ignore-timeouts --throttle 1000 -s 1 --pct-touch 60 30000000 > /data/logs/monkey.log&

#while function usage
function test_f()
{
	rm /storage/sdcard0/DCIM/Camera/*.jpg
	rm /storage/sdcard0/DCIM/Camera/*.mp4
	rm /storage/sdcard0/DCIM/Camera/*.3gp
	sleep 36000
}
#$1 times for while
#$2 the function to exec
function while_function_base()
{
	index=0;
	while [ 1 ]
	do
		if(($index<$1))
		then
			$2 $index
			index=$index+1;
		else
			break;
		fi
	done
}

function test_while_function_base()
{
	while_function_base 300000000 test_f
}

test_while_function_base

