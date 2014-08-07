
vir_value_path="/d/debug_mem/value";
vir_addr_path="/d/debug_mem/address";
phy_value_path="/d/debug_mem/phy_value";
phy_addr_path="/d/debug_mem/phy_addr";
count_path="/d/debug_mem/count";
phy_printmems="/d/debug_mem/phy_printmems";
vir_2_phy="/d/debug_mem/vir_2_phy";
phy_2_vir="/d/debug_mem/phy_2_vir";

function set_value_addr()
{
	echo $1 > $vir_addr_path; 
}

get_value_returned=-1;
function get_value()
{
	set_value_addr $1;
	get_value_returned=`cat $vir_value_path`;
}

function set_value()
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
		get_value $address;
		echo "value : $get_value_returned";
	else
		set_value $address $2;
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
