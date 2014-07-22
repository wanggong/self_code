function work_function()
{
		#endliss node
		input touchscreen tap 180 901
		sleep 2
		
		#give some body life 
		#may click err and mesgbox out , close it 
		input touchscreen tap 500 142
		sleep 1
		input touchscreen tap 281 868
		sleep 2
		#buy something
		input touchscreen tap 281 868
		sleep 2
		#wait util dead
		sleep 2

		#dead back 
		input touchscreen tap 190 536
		sleep 2
		#get the plane etc
		input touchscreen tap 273 705
		sleep 2
		#continue
		input touchscreen tap 180 879
		sleep 2
}

function while_function()
{
	index=0;
	while [ 1 ]
	do
		if(($index<5000))
		then
			work_function
			index=$index+1;
		else
			break;
		fi
	done
}

#work_function
while_function