
vir_value_path="/data/debugfs/debug_mem/value";
vir_addr_path="/data/debugfs/debug_mem/address";

function set_value_addr()
{
	echo $1 > $vir_addr_path; 
}

function get_value()
{
	set_value_addr $1;
	value=`cat $vir_value_path`;echo "value : $value";
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
	else
		set_value $address $2;
	fi
}

function base_help()
{
	echo "vir_value address (-value)";
}

base_help
