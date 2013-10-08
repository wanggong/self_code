
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

function set_gpio_intr_addr()
{
	let gpio_num=$1;
	let gpio_cfg_addr=0xfd511008+$gpio_num*0x10;
	echo $gpio_cfg_addr > $phy_addr_path;
}

function set_gpio_intr_status_addr()
{
	let gpio_num=$1;
	let gpio_cfg_addr=0xfd51100c+$gpio_num*0x10;
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

function get_gpio_intr()
{
	set_gpio_intr_addr $1;
	gpio_value=`cat $phy_value_path`;echo "gpio_value : $gpio_value";
	let dir_conn_en=gpio_value\>\>8\&0x1;echo "dir_conn_en : $dir_conn_en";
	let INTR_RAW_STATUS_EN=gpio_value\>\>4\&0x1;echo "INTR_RAW_STATUS_EN : $INTR_RAW_STATUS_EN";
	let INTR_DECT_CTL=gpio_value\>\>2\&0x1;echo "INTR_DECT_CTL : $INTR_DECT_CTL";
	let INTR_POL_CTL=gpio_value\>\>1\&0x1;echo "INTR_POL_CTL : $INTR_POL_CTL";
	let INTR_ENABLE=gpio_value\>\>0\&0x1;echo "INTR_ENABLE : $INTR_ENABLE";
}

function set_gpio_intr()
{
	set_gpio_intr_addr $1;
	echo "gpio_intr_value : $2";
	echo $2 > $phy_value_path;
}

function get_gpio_intr_status()
{
	set_gpio_intr_status_addr $1;
	gpio_value=`cat $phy_value_path`;echo "gpio_value : $gpio_value";
	let INTR_STATUS=gpio_value\>\>0\&0x1;echo "INTR_STATUS : $INTR_STATUS";
}


function gpio_help()
{
	echo "mkdir -p /data/debugfs;"
	echo "mount -t debugfs nodev /data/debugfs;"
	echo "then";
	echo "get_gpio_cfg gpio_num to get the config of the gpio";
	echo "set_gpio_cfg gpio_num gpio_cfg to set the config of the gpio";
	echo "get_gpio_value gpio_num to get the value of the gpio";
	echo "set_gpio_value gpio_num gpio_value to set the value of the gpio";
	echo "get_gpio_intr gpio_num to get the config of gpio interrupt";
	echo "set_gpio_intr gpio_num gpio_intr_value to set the gpio interrupt config value";
	echo "get_gpio_intr_status gpio_num to get the gpio interrupt status value";
}