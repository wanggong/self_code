
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

function read_onebyte_register()
{
	let opcode=0x01\<\<27;
	let sid=0\<\<20;
	let addr=$1\<\<4;
	let read_count=1-1;
	let send_cmd=$opcode\|$sid\|$addr\|$read_count;
	write_command $send_cmd;
	read_data;
	echo $read_data0;
}

function write_onebyte_register()
{
	let opcode=0x00\<\<27;
	let sid=0\<\<20;
	let addr=$1\<\<4;
	let read_count=1-1;
	let send_cmd=$opcode\|$sid\|$addr\|$read_count;
	write_data0 $2;
	write_command $send_cmd;
	read_data;
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
			read_onebyte_register $addr;
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


function help_pmic()
{
	echo "read_onebyte_register (reg_addr)to read one register";
	echo "write_onebyte_register (reg_addr value) write one reg";
}




