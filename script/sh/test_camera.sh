let file_count=0
function all_file()
{
	let file_count=0;
	for file in /storage/sdcard0/DCIM/Camera/*
	do
		let file_count=$file_count+1
	done
}

function takepicture()
{
	input keyevent 27
	sleep 3;
}
function startcamera()
{
#	am start -a android.media.action.IMAGE_CAPTURE
	sleep 3;
}
function exitcamera()
{
	input keyevent 3
}

#while function usage
function test_f()
{
	rm /storage/sdcard0/DCIM/Camera/*.jpg
	takepicture
	all_file
	if(($file_count<2))
	then
		error_time=`date`;
		echo $error_time>>/storage/sdcard0/DCIM/takepicture_error.txt
	fi
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
	logcat -v threadtime -n 1000 -r 50000 -f /storage/sdcard0/DCIM/logcat.log&
	cat /proc/kmsg > /storage/sdcard0/DCIM/kmsg.log&
	startcamera
	while_function_base 30000000 test_f
}

function take_picture_test()
{
	test_while_function_base
}

take_picture_test


