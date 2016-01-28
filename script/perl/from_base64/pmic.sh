#!/bin/bash
. base.sh;

#2716页

GET_CHANNEL_ADDRESS=0x0200F800
get_channel_config_returned=(1 2 3);
function get_channel_config()
{
	local index=1;
	while [ 1 ]
	do
		if(($index<=128))
		then
			let address_io_value=$GET_CHANNEL_ADDRESS+0x4*$index;
			io_value_get $address_io_value;
			get_channel_config_returned[$index]=$io_value_get_returned;
			let index=$index+1;
		else
			break;
		fi
	done
	#echo ${get_channel_config_returned[*]};
}

find_channel_index_returned=0;
function find_channel_index()
{
	let sid=$1;
	let addr=$2;
	let pid=$addr\>\>8;
	#echo "sid=$sid,pid=$pid";
	local index=1;
	let match_channel_value=$sid*0x10000+$pid*0x100;
	#echo "match_channel_value:$match_channel_value";
	while [ 1 ]
	do
		if(($index<=128))
		then
			#let ch_sid=get_channel_config_returned[$index]\>\>16\&0xF;
			#let ch_pid=get_channel_config_returned[$index]\>\>8\&0xFF;
			#echo "ch_sid=$ch_sid,ch_pid=$ch_pid";
			#echo "get_channel_config_returned = get_channel_config_returned[$index]";
			if(($match_channel_value==get_channel_config_returned[$index]))
			then
			{
				find_channel_index_returned=$index;
				#echo "finded";
				break;
			}
			fi
			let index=$index+1;
		else
			break;
		fi
	done
	#echo find_channel_index_returned = $find_channel_index_returned;
}

get_channel_offset_returned=0;
function update_channel_offset()
{
	let sid=$1\>\>16\&0xf;
	let addr=$1\&0xffff;
	find_channel_index $sid $addr;
	let get_channel_offset_returned=$find_channel_index_returned*0x8000+0*$1;
}



pmic_core_base_addr=0x02C00000;
write_base_addr=0x02400000;
cmd_offset=0;
config_offset=0x4;
status_offset=0x8;
wdata0_offset=0x10;
wdata1_offset=0x14;
rdata0_offset=0x18;
rdata1_offset=0x1c;
rdata2_offset=0x20;

#var
read_data0=0;
read_data1=0;
read_data2=0;
command_will_send=0;

# first parameter is the command

pmic_get_phy_address_returned=0；
function pmic_get_phy_address
{
	let pmic_get_phy_address_returned=$1+$get_channel_offset_returned+$2;
	#echo "address:$address";
	echo $pmic_get_phy_address > $phy_addr_path;
}
get_read_phy_address_returned=0;
function get_read_phy_address()
{
	pmic_get_phy_address $pmic_core_base_addr $1;
	get_read_phy_address_returned=$pmic_get_phy_address_returned;
}

get_write_phy_address_returned=0;
function get_write_phy_address()
{
	pmic_get_phy_address $write_base_addr $1;
	get_write_phy_address_returned=$pmic_get_phy_address_returned;
}

function write_read_command()
{
	get_read_phy_address $cmd_offset;
	io_value_set $get_read_phy_address_returned  $1;
}

function write_write_command()
{
	get_write_phy_address $cmd_offset
	#io_value_set $get_write_phy_address_returned  $1;
	parameter_format_io_address_array[$parameter_format_io_count]=$get_write_phy_address_returned;
	parameter_format_io_value_array[$parameter_format_io_count]=$1;
	let parameter_format_io_count=$parameter_format_io_count+1;
}

function write_data0()
{
	get_write_phy_address $wdata0_offset
	io_value_set $get_write_phy_address_returned $1;
}

function update_data0_array()
{
	get_write_phy_address $wdata0_offset;
	parameter_format_io_address_array[$parameter_format_io_count]=$get_write_phy_address_returned;
	parameter_format_io_value_array[$parameter_format_io_count]=$1;
	let parameter_format_io_count=$parameter_format_io_count+1;
}

#function write_data1()
#{
#	get_write_phy_address $wdata1_offset
#	io_value_set $get_write_phy_address_returned  $1;
#}


function read_data()
{
	get_read_phy_address $rdata0_offset;
	io_value_get $get_read_phy_address_returned;
	read_data0=$io_value_get_returned;
	
	get_read_phy_address $rdata1_offset
	io_value_get $get_read_phy_address_returned;
	read_data1=$io_value_get_returned;
	
	get_read_phy_address $rdata2_offset
	io_value_get $get_read_phy_address_returned;
	read_data1=$io_value_get_returned;
}

#$1 0 for write 1 for read
#$2 sid charging is 0 , ldo is 2
#$3 address
#$4 count
function composite_read_command_and_send()
{
	let opcode=$1\<\<27;
	let addr=\($3\&0xffff\);
	let sid=$3\>\>16;
	let read_count=$4-1;
	#echo "sid=$sid,addr=$addr";
	let addr=addr\<\<4;
	let command_will_send=$opcode\|$sid\|$addr\|$read_count;
	write_read_command $command_will_send;
}

function update_write_command()
{
	let opcode=$1\<\<27;
	let addr=\($3\&0xffff\);
	let sid=$3\>\>16;
	let read_count=$4-1;
	#echo "sid=$sid,addr=$addr";
	let addr=addr\<\<4;
	let command_will_send=$opcode\|$sid\|$addr\|$read_count;
	write_write_command $command_will_send;
}

function pmic_read1()
{
	#echo "before update_channel_offset";
	update_channel_offset $1;
	#echo "before composite_read_command_and_send";
	composite_read_command_and_send 0x01 0x0 $1 0x1;
	#echo "before read_data";
	read_data;
	echo $read_data0;
}

#$1 is the reg address
#$2 is the reg value
function pmic_update_write_array()
{
	update_channel_offset $1;
	#write_data0 $2;
	update_data0_array $2;
	update_write_command 0x0 0x0 $1 0x1;
}

function pmic_write1()
{
	pmic_update_write_array $1 $2;
	write_io_format_value_all;
	parameter_format_io_count=0;
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

function power_key_dl()
{
	pmic_reg 0x840 0;
	pmic_reg 0x841 0;
	pmic_reg 0x842 1;
}

function battery_only()
{
	#echo 0 > /sys/class/power_supply/battery/charging_enabled;
	write_value echo 0 "/sys/class/power_supply/battery/charging_enabled";
	pmic_reg 0x1049 1;
	#echo 'file qpnp-adc-common.c +p' > /d/dynamic_debug/control
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
get_ldo_enable_address_returned=0;
function get_ldo_enable_address()
{
	let ldo_index=$1-1;
	let get_ldo_enable_address_returned=$LDO_BASE_START+$ldo_index*0x100+$ENABLE_REG_ADDR;
}
function ldo_enable()
{
	get_ldo_enable_address $1;
	let ldo_reg_addr=$get_ldo_enable_address_returned;
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

function rtc_time()
{
	pimc_reg 0x604B;
	pimc_reg 0x604A;
	pimc_reg 0x6049;
	pimc_reg 0x6048;
}

function alarm_time()
{
	pimc_reg 0x6143;
	pimc_reg 0x6142;
	pimc_reg 0x6141;
	pimc_reg 0x6140;
}



function help_pmic()
{
	echo "----------------------PMIC--------------------------------";
	echo "pmic_reg (reg_addr -value) read or write pmic reg";
	echo "ldo_enable (reg_addr -value) read or write ldo enable reg";
	echo "ldo_mode (reg_addr -value) read or write ldo mode reg";
	echo "power_key_dl press power key ,enter download mode immediately"
	echo "ldo_status_all print all ldo status";
	echo " battery_only set battery only , not use usb power";
	echo " rtc_time show rtc register(seconds)";
	echo " alarm_time show alarm register(seconds)";
	echo "----------------------------------------------------------";
}
get_channel_config;
help_pmic;




