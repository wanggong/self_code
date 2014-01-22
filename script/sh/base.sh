
vir_value_path="/data/debugfs/debug_mem/value";
vir_addr_path="/data/debugfs/debug_mem/address";
phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";
count_path="/data/debugfs/debug_mem/count";
phy_printmems="/data/debugfs/debug_mem/phy_printmems";
vir_2_phy="/data/debugfs/debug_mem/vir_2_phy";
phy_2_vir="/data/debugfs/debug_mem/phy_2_vir";

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

function io_value()
{
	echo $1 > $phy_addr_path;
	if(($#<2))
	then
		phy_value=`cat $phy_value_path`;
		echo $phy_value;
	else
		echo $2 > $phy_value_path;
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
	echo "vir_value address (-value)";
	echo "io_value address (-value)";
	echo "vir_to_phy value (convert virtual to phy)"
	echo "phy_to_vir value (convert phy to virtual)"
}

base_help
