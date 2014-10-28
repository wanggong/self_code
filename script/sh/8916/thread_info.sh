
. base.sh
function thread_info_config()
{
	if(($#<1))
	then
		cat $thread_info_thread_info_path;
	else
		echo $1 > $thread_info_thread_info_path;
	fi
}

function print_seconds()
{
	if(($#<1))
	then
		cat $thread_info_print_seconds_path;
	else
		echo $1 > $thread_info_print_seconds_path;
	fi
}

function schedule_seconds()
{
	if(($#<1))
	then
		cat $thread_info_schedule_seconds_path;
	else
		echo $1 > $thread_info_schedule_seconds_path;
	fi
}


function help_thread_info()
{
	echo "----------------------thread_info--------------------------------";
	echo "thread_info_config (kernel for kernel ,or other filename)";
	echo "print_seconds (-value) read or write the print seconds";
	echo "schedule_seconds (-value) read or write the schedule seconds";
	echo "----------------------------------------------------------";
}
help_thread_info;




