function startcamera()
{
	am start org.codeaurora.snapcam
	sleep 1;
}
function exitcamera()
{
	input keyevent 3
	sleep 1;
}

function takepicture()
{
	input keyevent 27
	sleep 2;
}

#while function usage
function test_f()
{
	startcamera;
	takepicture;
	exitcamera;
	ps mediaserver;
	dumpsys meminfo mediaserver;
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
	while_function_base 10000 test_f
}

test_while_function_base



