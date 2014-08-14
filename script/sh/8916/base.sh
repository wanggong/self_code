
vir_value_path="/d/debug_mem/value";
vir_addr_path="/d/debug_mem/address";
phy_value_path="/d/debug_mem/phy_value";
phy_addr_path="/d/debug_mem/phy_addr";
count_path="/d/debug_mem/count";
phy_printmems="/d/debug_mem/phy_printmems";
vir_2_phy="/d/debug_mem/vir_2_phy";
phy_2_vir="/d/debug_mem/phy_2_vir";
suspend_resume_addr="/d/debug_mem/suspend_resume/suspend_resume_addr";
suspend_resume_test="/d/debug_mem/suspend_resume/test";

function set_value_addr()
{
	echo $1 > $vir_addr_path; 
}

vir_value_get_returned=-1;
function vir_value_get()
{
	set_value_addr $1;
	vir_value_get_returned=`cat $vir_value_path`;
}

function vir_value_set()
{
	set_value_addr $1;
	#echo "value : $2";
	echo $2 > $vir_value_path;
}


function vir_value()
{
	let address=$1;
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
	echo $1 > $phy_addr_path;
	io_value_get_returned=`cat $phy_value_path`;
}
function io_value_set()
{
	echo $1 > $phy_addr_path;
	echo $2 > $phy_value_path;
}
function io_value()
{
	echo $1 > $phy_addr_path;
	if(($#<2))
	then
		io_value_get $1;
		echo "value:$io_value_get_returned";
	else
		io_value_set $1 $2;
	fi
}

function vir_to_phy()
{
	echo $1 > $vir_2_phy;
	phy_value=`cat $vir_2_phy`;
	echo $phy_value;
}

function phy_to_vir()
{
	echo $1 > $phy_2_vir;
	vir_value=`cat $phy_2_vir`;
	echo $vir_value;
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
write_vir_format_value_one()
{
	vir_value_set ${parameter_format_vir_address_array[$1]} ${parameter_format_vir_value_array[$1]} 
}

read_vir_format_value_one()
{
	vir_value_get ${parameter_format_vir_address_array[$1]};
	parameter_format_vir_read_array[$1]=vir_value_get_returned;
}

#$1 is the index
write_io_format_value_one()
{
	io_value_set ${parameter_format_io_address_array[$1]} ${parameter_format_io_value_array[$1]} 
}

read_io_format_value_one()
{
	io_value_get ${parameter_format_io_address_array[$1]};
	parameter_format_io_read_array[$1]=$io_value_get_returned;
}

write_vir_format_value_all()
{
	while_function_base $parameter_format_vir_count write_vir_format_value_one
}

read_vir_format_value_all()
{
	while_function_base $parameter_format_vir_count read_vir_format_value_one
}

write_io_format_value_all()
{
	while_function_base $parameter_format_io_count write_io_format_value_one
}

read_io_format_value_all()
{
	while_function_base $parameter_format_io_count read_io_format_value_one
}


function base_help()
{
	echo "--------------------------------------------";
	echo "vir_value address (-value)";
	echo "io_value address (-value)";
	echo "vir_to_phy value (convert virtual to phy)"
	echo "phy_to_vir value (convert phy to virtual)"
	echo "--------------------------------------------";
}

base_help
