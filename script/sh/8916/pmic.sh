
phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";
count_path="/data/debugfs/debug_mem/count";
phy_printmems="/data/debugfs/debug_mem/phy_printmems";

#2716页

GET_CHANNEL_ADDRESS=0x0200F800
channel_array=(1 2 3);
IO_VALUE=0;
function io_value()
{
	echo $1 > $phy_addr_path;
	if(($#<2))
	then
		IO_VALUE=`cat $phy_value_path`;
		#echo $IO_VALUE;
	fi
}

function get_channel()
{
	local index=1;
	while [ 1 ]
	do
		if(($index<=128))
		then
			let address_io_value=$GET_CHANNEL_ADDRESS+0x4*$index;
			io_value $address_io_value;
			channel_array[$index]=$IO_VALUE;
			let index=$index+1;
		else
			break;
		fi
	done
	#echo ${channel_array[*]};
}

channel_index=0;
function find_channel()
{
	let sid=$1;
	let addr=$2;
	let pid=$addr\>\>8;
	echo "sid=$sid,pid=$pid";
	get_channel;
	local index=1;
	let match_channel=$sid*0x10000+$pid*0x100;
	echo "match_channel:$match_channel";
	while [ 1 ]
	do
		if(($index<=128))
		then
			let ch_sid=channel_array[$index]\>\>16\&0xF;
			let ch_pid=channel_array[$index]\>\>8\&0xFF;
			echo "ch_sid=$ch_sid,ch_pid=$ch_pid";
			echo "channel_array = channel_array[$index]";
			if(($match_channel==channel_array[$index]))
			then
			{
				channel_index=$index;
				echo "finded";
				break;
			}
			fi
			let index=$index+1;
		else
			break;
		fi
	done
	echo channel_index = $channel_index;
}

channel_offset=0;
function find_channel_offset()
{
	let sid=$1\>\>16\&0xf;
	let addr=$1\&0xffff;
	find_channel $sid $addr;
	let channel_offset=$channel_index*0x8000+0*$1;
}



base_addr=0x02C00000;
write_base_addr=0x02400000;
channel_max=6;
regs_per_channel=0x8000;
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
	let address=$base_addr+$channel_offset+$1;
	echo "address:$address";
	echo $address > $phy_addr_path;
}

function set_write_phy_address()
{
	let address=$write_base_addr+$channel_offset+$1;
#	echo "address:$address";
	echo $address > $phy_addr_path;
}

function write_command()
{
	set_phy_address $cmd_offset
	echo $1 > $phy_value_path;
}

function write_write_command()
{
	set_write_phy_address $cmd_offset
	echo $1 > $phy_value_path;
}

function write_data0()
{
	set_write_phy_address $wdata0_offset
	echo $1 > $phy_value_path;
}

function write_data1()
{
	set_write_phy_address $wdata1_offset
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
	let addr=\($3\&0xffff\);
	let sid=$3\>\>16;
	let read_count=$4-1;
	echo "sid=$sid,addr=$addr";
	let addr=addr\<\<4;
	let command_will_send=$opcode\|$sid\|$addr\|$read_count;
	write_command $command_will_send;
}

function write_composite_and_write_command()
{
	let opcode=$1\<\<27;
	let addr=\($3\&0xffff\);
	let sid=$3\>\>16;
	let read_count=$4-1;
	echo "sid=$sid,addr=$addr";
	let addr=addr\<\<4;
	let command_will_send=$opcode\|$sid\|$addr\|$read_count;
	write_write_command $command_will_send;
}

function pmic_read1()
{
	find_channel_offset $1;
	composite_and_write_command 0x01 0x0 $1 0x1;
	read_data;
	echo $read_data0;
}

function pmic_write1()
{
	find_channel_offset $1;
	write_data0 $2;
	write_composite_and_write_command 0x0 0x0 $1 0x1;
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
LDO_COUNT=18；
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
		if(($index<=$LDO_COUNT))
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




