
. base.sh;

GPIO_BASE_ADDRESS=0x01000000;

#地址获取文档：MSM8916 Sofware Interface for OEMs
#80-NK807-2x Rev. B March 
#20143125页 
#mkdir -p /data/debugfs;
#mount -t debugfs nodev /data/debugfs;

get_gpio_cfg_addr_returned=0;
function get_gpio_cfg_addr()
{
	let gpio_num=$1;
	let get_gpio_cfg_addr_returned=$GPIO_BASE_ADDRESS+$gpio_num*0x1000;
	#echo $get_gpio_cfg_addr_returned > $phy_addr_path;
}

get_gpio_value_addr_returned=0;
function get_gpio_value_addr()
{
	let gpio_num=$1;
	let get_gpio_value_addr_returned=$GPIO_BASE_ADDRESS+0x4+$gpio_num*0x1000;
}

get_gpio_intr_addr_returned=0;
function get_gpio_intr_addr()
{
	let gpio_num=$1;
	let get_gpio_intr_addr_returned=$GPIO_BASE_ADDRESS+0x8+$gpio_num*0x1000;
	#echo $gpio_cfg_addr > $phy_addr_path;
}

get_gpio_intr_status_addr_returned=0;
function get_gpio_intr_status_addr()
{
	let gpio_num=$1;
	let get_gpio_intr_status_addr_returned=$GPIO_BASE_ADDRESS+0xC+$gpio_num*0x1000;
	#echo $gpio_cfg_addr > $phy_addr_path;
}

function get_gpio_cfg()
{
	get_gpio_cfg_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_cfg_addr_returned;
	parameter_format_io_count=1;
	read_io_format_value_all;
	parameter_format_io_count=0;
	gpio_value=${parameter_format_io_read_array[0]};
	echo "gpio_cfg_value : $gpio_value";
	let hihys_en=gpio_value\>\>10\&0x1;echo "hihys_en : $hihys_en";
	let gpio_oe=gpio_value\>\>9\&0x1;echo "gpio_oe : $gpio_oe";
	let gpio_drv_strengh=gpio_value\>\>6\&0x7;echo "gpio_drv_strengh : $gpio_drv_strengh";
	let func=gpio_value\>\>2\&0xf;echo "func : $func";
	let gpio_pull=gpio_value\>\>0\&0x3;echo "gpio_pull : $gpio_pull";
}

function set_gpio_cfg()
{
	get_gpio_cfg_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_cfg_addr_returned;
	echo "gpio_cfg_value : $2";
	parameter_format_io_value_array[0]=$2;
	parameter_format_io_count=1;
	write_io_format_value_all;
	parameter_format_io_count=0;
}

function get_gpio_value()
{
	#set_gpio_value_addr $1;
	get_gpio_value_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_value_addr_returned;
	parameter_format_io_count=1;
	read_io_format_value_all;
	parameter_format_io_count=0;
	gpio_value=${parameter_format_io_read_array[0]};
	
	#gpio_value=`cat $phy_value_path`;echo "gpio_value : $gpio_value";
	let gpio_out=gpio_value\>\>1\&0x1;echo "gpio_out : $gpio_out";
	let gpio_in=gpio_value\>\>0\&0x1;echo "gpio_in : $gpio_in";
}

function set_gpio_value()
{
	get_gpio_value_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_value_addr_returned;
	echo "gpio_out_value : $2";
	parameter_format_io_value_array[0]=$2;
	parameter_format_io_count=1;
	write_io_format_value_all;
	parameter_format_io_count=0;
}


function get_gpio_intr()
{
	get_gpio_intr_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_intr_addr_returned;
	parameter_format_io_count=1;
	read_io_format_value_all;
	parameter_format_io_count=0;
	gpio_value=${parameter_format_io_read_array[0]};
	
	echo "gpio_value : $gpio_value";
	let dir_conn_en=gpio_value\>\>8\&0x1;echo "dir_conn_en : $dir_conn_en";
	let INTR_RAW_STATUS_EN=gpio_value\>\>4\&0x1;echo "INTR_RAW_STATUS_EN : $INTR_RAW_STATUS_EN";
	let INTR_DECT_CTL=gpio_value\>\>2\&0x1;echo "INTR_DECT_CTL : $INTR_DECT_CTL";
	let INTR_POL_CTL=gpio_value\>\>1\&0x1;echo "INTR_POL_CTL : $INTR_POL_CTL";
	let INTR_ENABLE=gpio_value\>\>0\&0x1;echo "INTR_ENABLE : $INTR_ENABLE";
}

function set_gpio_intr()
{
	get_gpio_intr_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_intr_addr_returned;
	echo "gpio_intr_value : $2";
	parameter_format_io_value_array[0]=$2;
	parameter_format_io_count=1;
	write_io_format_value_all;
	parameter_format_io_count=0;
}

function get_gpio_intr_status()
{
	get_gpio_intr_status_addr $1;
	parameter_format_io_address_array[0]=$get_gpio_intr_status_addr_returned;
	parameter_format_io_count=1;
	read_io_format_value_all;
	parameter_format_io_count=0;
	gpio_value=${parameter_format_io_read_array[0]};
	
	echo "gpio_value : $gpio_value";
	let INTR_STATUS=gpio_value\>\>0\&0x1;echo "INTR_STATUS : $INTR_STATUS";
}


function gpio_value()
{
	if(($#<2))
	then
		get_gpio_value $1;
	else
		set_gpio_value $1 $2;
	fi
}

function gpio_cfg()
{
	if(($#<2))
	then
		get_gpio_cfg $1;
	else
		set_gpio_cfg $1 $2;
	fi
}

function gpio_intr()
{
	if(($#<2))
	then
		get_gpio_intr $1;
	else
		set_gpio_intr $1 $2;
	fi
}

function gpio_help()
{
	echo "------------------------------------GPIO----------------------------------------------";
	echo "gpio_cfg (gpio_num -gpio_cfg) to get or set the config of the gpio";
	echo "gpio_value (gpio_num -gpio_value) to get or set the value of the gpio";
	echo "gpio_intr (gpio_num -gpio_intr_value) to get or set the gpio interrupt config value";
	echo "get_gpio_intr_status gpio_num to get the gpio interrupt status value";
	echo "----------------------------------------------------------------------------------------";
}
gpio_help;