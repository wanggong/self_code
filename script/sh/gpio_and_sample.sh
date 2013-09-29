function func_test()
{
a="wang";  #there is no space between 'a' and '=' and 'wang' 
echo "wang";
}
#warning there is no brace'()' after the function name;use the function name to call function directly
function func_usage()
{
	func_test;
}
#func parameters test
# $# is the count of the parameters , 
function func_param_test
{
	echo "parameter count is $#" ;
#	for((i=0;$i<$#;i++))
	{
		eval b=\$$i;	
		echo $b;
	}
}
function func_param_usage()
{
	func_param_test wang li zhao zhang;
}

#input parameters , use $1,$2 ,...... to get it
#echo $1;
#echo $2l
#echo $3;


phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";

#mkdir -p /data/debugfs;
#mount -t debugfs nodev /data/debugfs;
function set_gpio_cfg_addr()
{
	let gpio_num=$1;
	let gpio_cfg_addr=0xfd511000+$gpio_num*0x10;
	echo $gpio_cfg_addr > $phy_addr_path;
}

function set_gpio_value_addr()
{
	let gpio_num=$1;
	let gpio_cfg_addr=0xfd511004+$gpio_num*0x10;
	echo $gpio_cfg_addr > $phy_addr_path;
}

function get_gpio_cfg()
{
set_gpio_cfg_addr $1;
gpio_value=`cat $phy_value_path`;echo "gpio_value : $gpio_value";
let hihys_en=gpio_value\>\>10\&0x1;echo "hihys_en : $hihys_en";
let gpio_oe=gpio_value\>\>9\&0x1;echo "gpio_oe : $gpio_oe";
let func=gpio_value\>\>2\&0xf;echo "func : $func";
let gpio_pull=gpio_value\>\>0\&0x3;echo "gpio_pull : $gpio_pull";
}

function set_gpio_cfg()
{
set_gpio_cfg_addr $1;
echo "gpio_cfg_value : $2";
echo $2 > $phy_value_path;
}

function get_gpio_value()
{
set_gpio_value_addr $1;
gpio_value=`cat $phy_value_path`;echo "gpio_value : $gpio_value";
let gpio_out=gpio_value\>\>1\&0x1;echo "gpio_out : $gpio_out";
let gpio_in=gpio_value\>\>0\&0x1;echo "gpio_in : $gpio_in";
}

function set_gpio_value()
{
set_gpio_value_addr $1;
echo "gpio_out_value : $2";
echo $2 > $phy_value_path;
}


function gpio_help()
{
	echo "mkdir -p /data/debugfs;"
	echo "mount -t debugfs nodev /data/debugfs;"
	echo "then";
	echo "get_gpio_cfg gpio_num to get the config of the gpio"
	echo "set_gpio_cfg gpio_num gpio_cfg to set the config of the gpio"
	echo "get_gpio_value gpio_num to get the value of the gpio"
	echo "set_gpio_value gpio_num gpio_value to set the value of the gpio"
}