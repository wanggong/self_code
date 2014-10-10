
#while function usage
function test_f()
{
	input touchscreen tap 100 200
	sleep 2
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


