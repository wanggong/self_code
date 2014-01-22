
phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";
count_path="/data/debugfs/debug_mem/count";
phy_printmems="/data/debugfs/debug_mem/phy_printmems";

base_addr=0xfc4cf800;
channel_max=6;
regs_per_channel=0x80;
cmd_offset=0;
config_offset=0x4;
status_offset=0x8;
wdata0_offset=0x10;
wdata1_offset=0x14;
rdata0_offset=0x18;
rdata1_offset=0x1c;
rdata2_offset=0x20;

#var
channel=0;
read_data0=0;
read_data1=0;
read_data2=0;
command_will_send=0;

# first parameter is the command
function set_phy_address()
{
	let address=$base_addr+$regs_per_channel*$channel+$1;
#	echo "address:$address";
	echo $address > $phy_addr_path;
}
function write_command()
{
	set_phy_address $cmd_offset
	echo $1 > $phy_value_path;
}

function write_data0()
{
	set_phy_address $wdata0_offset
	echo $1 > $phy_value_path;
}

function write_data1()
{
	set_phy_address $wdata1_offset
	echo $1 > $phy_value_path;
}


function read_data()
{
	set_phy_address $rdata0_offset;
	read_data0=`cat $phy_value_path`;
	set_phy_address $rdata1_offset
	read_data1=`cat $phy_value_path`;
	set_phy_address $rdata2_offset
	read_data2=`cat $phy_value_path`;
}

#$1 0 for write 1 for read
#$2 sid charging is 0 , ldo is 2
#$3 address
#$4 count
function composite_and_write_command()
{
	let opcode=$1\<\<27;
	let sid=$2\<\<20;
	let addr=$3\<\<4;
	let read_count=$4-1;
	let command_will_send=$opcode\|$sid\|$addr\|$read_count;
	write_command $command_will_send;
}

function pmic_read1()
{
	composite_and_write_command 0x01 0x0 $1 0x1;
	read_data;
	echo $read_data0;
}

function pmic_write1()
{
	write_data0 $2;
	composite_and_write_command 0x0 0x0 $1 0x1;
}

function pmic_reg()
{
	if (($#>1))
	then
	pmic_write1 $1 $2
	else
	pmic_read1 $1
	fi
}

function battery_only()
{
	echo 0 > /sys/class/power_supply/battery/charging_enabled;
	pmic_reg 0x1049 1;
	echo 'file qpnp-adc-common.c +p' > /data/debugfs/dynamic_debug/control
}



CHGR_ADDRESS=0x1000;
BUCK_ADDRESS=0x1100;
BAT_IF_ADDRESS=0x1200;
USB_CHGPATH_ADDRESS=0x1300;
MISC_ADDRESS=0x1600;


function print_regs()
{
	local array_value;
		
	local index=0;
	while [ 1 ]
	do
		if(($index<$2))
		then
			let addr=$1+$index;
			pmic_read1 $addr;
			let array_index=$index%16;
			array_value[$array_index]=$read_data0;
			if((array_index==15))
			then
				echo ${array_value[*]};
			fi
			let index=$index+1;
		else
			break;
		fi
	done
}


function read_all_charger_reg()
{
	echo "CHGR REGS";
	print_regs $CHGR_ADDRESS 0x100;
	echo "BUCK REGS";
	print_regs $BUCK_ADDRESS 0x100;
	echo "BAT_IF REGS";
	print_regs $BAT_IF_ADDRESS 0x100;
	echo "USB_CHGPATH REGS";
	print_regs $USB_CHGPATH_ADDRESS 0x100;
	echo "MISC REGS";
	print_regs $MISC_ADDRESS 0x100;

}


let LDO_BASE_START=0x14000;
ONE_LDO_REGS=0x100;
ENABLE_REG_ADDR=0x46;
MODE_REG_ADDR=0x45;
function ldo_enable()
{
	let ldo_index=$1-1;
	let ldo_reg_addr=$LDO_BASE_START+$ldo_index*0x100+$ENABLE_REG_ADDR;
	pmic_read1 $ldo_reg_addr;
	if (($#>1))
	then
	pmic_write1 $ldo_reg_addr $2;
	pmic_read1 $ldo_reg_addr;
	fi
}

function ldo_mode()
{
	let ldo_index=$1-1;
	let ldo_reg_addr=$LDO_BASE_START+$ldo_index*0x100+$MODE_REG_ADDR;
	pmic_read1 $ldo_reg_addr;
	if (($#>1))
	then
	pmic_write1 $ldo_reg_addr $2;
	pmic_read1 $ldo_reg_addr;
	fi
}

function ldo_status()
{
	let ldo_index=$1-1;
	let ldo_reg_addr=$LDO_BASE_START+$ldo_index*0x100+$MODE_REG_ADDR;
	pmic_read1 $ldo_reg_addr;
	echo "ldo_$1_mode:$read_data0";
	let ldo_reg_addr=$LDO_BASE_START+$ldo_index*0x100+$ENABLE_REG_ADDR;
	pmic_read1 $ldo_reg_addr;
	echo "ldo_$1_enable:$read_data0";
}

function ldo_status_all()
{
	local index=1;
	while [ 1 ]
	do
		if(($index<=22))
		then
			ldo_status $index
		else
			break;
		fi
		let index=$index+1;
	done
}


function help_pmic()
{
	echo "pmic_reg (reg_addr -value) read or write pmic reg";
	echo "ldo_enable (reg_addr -value) read or write ldo enable reg";
	echo "ldo_mode (reg_addr -value) read or write ldo mode reg";
	echo "ldo_status_all print all ldo status";
	echo " battery_only set battery only , not use usb power";
}

help_pmic;




