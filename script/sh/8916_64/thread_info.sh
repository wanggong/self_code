
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

function kernel_dead()
{
	cat $thread_info_kernel_dead;
}

function dump_pid()
{
	echo $1 > $thread_info_dump_pid;
}

function breakpoint_set()
{
        echo $1 > $breakpoint;
}

function breakpoint_mask()
{
	echo $1 > $hw_breakpoint_xwr;
}

function dump_stack_breakpoint()
{
        if(($#<1))
        then
                cat $dump_stack_breakpoint_path;
        else
                echo $1 > $dump_stack_breakpoint_path;
        fi
}


function help_thread_info()
{
	echo "----------------------thread_info--------------------------------";
	echo "thread_info_config (kernel for kernel ,or other filename)";
	echo "print_seconds (-value) read or write the print seconds";
	echo "schedule_seconds (-value) read or write the schedule seconds";
	echo "kernel_dead dump dead kernel thread backtrace";
	echo "dump_pid pid dump the pid backtrace";
	echo "breakpoint_set set breakpoint";
	echo "breakpoint_mask (1 load , 2 write ,4 exec)"
	echo "dump_stack_breakpoint (value)(1 dump backtrace in before function , 2 dump in after"
	echo "----------------------------------------------------------";
}
help_thread_info;




