
$base_path="/d/debug_wgz/";
$memory_path=$base_path"/memory/"
$vir_value_path=$memory_path"/vir_value";
$vir_addr_path=$memory_path"/vir_address";
$phy_value_path=$memory_path"/phy_value";
$phy_addr_path=$memory_path"/phy_address";
$taskpid_path=$memory_path"/taskpid";
$count_path="/d/debug_mem/count";
$phy_printmems="/d/debug_mem/phy_printmems";
$vir_2_phy="/d/debug_mem/vir_2_phy";
$phy_2_vir="/d/debug_mem/phy_2_vir";

$suspend_resume_dir=$base_path"/suspend_resume"
$suspend_resume_addr=$suspend_resume_dir"/suspend_resume_addr";
$suspend_resume_test=$suspend_resume_dir"test";
$suspend_resume_debug_path=$suspend_resume_dir"/suspend_resume_debug";

$thread_info_path=$base_path"/thread/"
$thread_info_print_seconds_path=$thread_info_path"/print_seconds";
$thread_info_schedule_seconds_path=$thread_info_path"/schedule_seconds";
$thread_info_kernel_dead=$thread_info_path"/dump_dead_kernel";
$thread_info_dump_pid=$thread_info_path"/dump_pid";

$breakpoint="/d/debug_mem/breakpoint/breakpoint";
$hw_breakpoint_xwr="/d/debug_mem/breakpoint/hw_breakpoint_xwr";
$dump_stack_breakpoint_path="/d/debug_mem/breakpoint/dump_stack_breakpoint"
$sound_register_address_path=$base_path"/sound/register_address";
$sound_register_value_path=$base_path"/sound/register_value";
function write_value()
{
	adb shell "echo $1 > $2";
}
read_value_returned="0";
function read_value()
{
	read_value_returned=`adb shell "cat $1"` ;
}
function set_value_addr()
{
#	adb shell "echo $1 > $vir_addr_path"; 
	write_value $1 $vir_addr_path;
}

vir_value_get_returned=-1;
function vir_value_get()
{
	set_value_addr $1;
#	vir_value_get_returned=`cat $vir_value_path`;
	read_value $vir_value_path;
	vir_value_get_returned=$read_value_returned;
}

function vir_value_set()
{
	set_value_addr $1;
	#echo "value : $2";
#	adb shell "echo $2 > $vir_value_path";
	write_value $2 $vir_value_path; 
}


function vir_value()
{
	address=$1;
	if(($#<2))
	then
		vir_value_get $address;
		echo "value : $vir_value_get_returned";
	else
		vir_value_set $address $2;
	fi
}

io_value_get_returned=-1;
function io_value_get()
{
	#echo $1 > $phy_addr_path;
	write_value $1 $phy_addr_path;
	read_value $phy_value_path;
	io_value_get_returned=vir_value_get_returned;
	#io_value_get_returned=`cat $phy_value_path`;
}
function io_value_set()
{
	#echo $1 > $phy_addr_path;
	#echo $2 > $phy_value_path;
	write_value $1 $phy_addr_path;
	write_value $2 $phy_value_path;
}
function io_value()
{
	#echo $1 > $phy_addr_path;
	write_value $1 $phy_addr_path;
	if(($#<2))
	then
		io_value_get $1;
		echo value:$io_value_get_returned;
	else
		io_value_set $1 $2;
	fi
}

function taskpid()
{
	if(($#<1))
	then
		#cat $taskpid_path
		read_value $taskpid_path;
		echo $read_value_returned
	else
		#echo $1 > $taskpid_path; 
		write_value $1 $taskpid_path; 
	fi
}

function vir_to_phy()
{
	#echo $1 > $vir_2_phy;
	write_value $1 $vir_2_phy;
	#phy_value=`cat $vir_2_phy`;
	#echo $phy_value;
	read_value $vir_2_phy;
	echo $read_value_returned;
}

function phy_to_vir()
{
	#echo $1 > $phy_2_vir;
	write_value $1 $phy_2_vir;
	#vir_value=`cat $phy_2_vir`;
	#echo $vir_value;
	read_value $phy_2_vir;
	echo $read_value_returned;
}

parameter_format_vir_count=0;
parameter_format_vir_address_array=(0,0);
parameter_format_vir_value_array=(0,0);
parameter_format_vir_read_array=(0,0);

parameter_format_io_count=0;
parameter_format_io_address_array=(0,0);
parameter_format_io_value_array=(0,0);
parameter_format_io_read_array=(0,0);

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

#$1 is the index
function write_vir_format_value_one()
{
	vir_value_set ${parameter_format_vir_address_array[$1]} ${parameter_format_vir_value_array[$1]} 
}

function read_vir_format_value_one()
{
	vir_value_get ${parameter_format_vir_address_array[$1]};
	parameter_format_vir_read_array[$1]=vir_value_get_returned;
}

#$1 is the index
function write_io_format_value_one()
{
	io_value_set ${parameter_format_io_address_array[$1]} ${parameter_format_io_value_array[$1]} 
}

function read_io_format_value_one()
{
	io_value_get ${parameter_format_io_address_array[$1]};
	parameter_format_io_read_array[$1]=$io_value_get_returned;
}

function write_vir_format_value_all()
{
	while_function_base $parameter_format_vir_count write_vir_format_value_one
}

function read_vir_format_value_all()
{
	while_function_base $parameter_format_vir_count read_vir_format_value_one
}

function write_io_format_value_all()
{
	while_function_base $parameter_format_io_count write_io_format_value_one
}

function read_io_format_value_all()
{
	while_function_base $parameter_format_io_count read_io_format_value_one
}

function ignore_loglevel()
{
	#echo $1 > /sys/module/printk/parameters/ignore_loglevel;
	write_value $1 "/sys/module/printk/parameters/ignore_loglevel";
}

function console_suspend()
{
	#echo $1 > /sys/module/printk/parameters/console_suspend;
	write_value $1 "/sys/module/printk/parameters/console_suspend";
}

function debug_clks()
{
	#echo $1 > /d/clk/debug_suspend;
	write_value $1 "/d/clk/debug_suspend";
}




function base_help()
{
	echo "------------------BASE--------------------";
	echo "vir_value address (-value)";
	echo "io_value address (-value)";
	echo "taskpid_path (-value)"
#	echo "vir_to_phy value (convert virtual to phy)"
#	echo "phy_to_vir value (convert phy to virtual)"
	echo "ignore_loglevel (1 ignore , 0 not ignore)"
	echo "console_suspend (1 suspend , 0 not suspend)"
	echo "debug_clks -value (1 enable 0 disable)"
	echo "--------------------------------------------";
}

base_help
ignore_loglevel 1

