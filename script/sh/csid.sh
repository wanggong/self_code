
phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";
count_path="/data/debugfs/debug_mem/count";
phy_printmems="/data/debugfs/debug_mem/phy_printmems";

csid1_base=0xfda00000;
csid2_base=0xfda00400;
csid_reg_length=0xd8;


function set_csid_addr()
{
	
	let csid_index=$1;
	let addr=$2;
	let set_addr=$csid1_base+$csid_index*0x400+addr;
	echo $set_addr > $phy_addr_path;
}

function print_csid_status()
{
	if(($#<1))
	then
		echo "need set which csid"
		return;
	fi
	set_csid_addr $1 0x8c;
	TOTAL_PKTS_RCVD=`cat $phy_value_path`;echo "TOTAL_PKTS_RCVD : $TOTAL_PKTS_RCVD";
	set_csid_addr $1 0x90;
	STATS_ECC=`cat $phy_value_path`;
	let TOTAL_UNRECOVERABLE_PKTS=$STATS_ECC\>\>16; echo "TOTAL_UNRECOVERABLE_PKTS:$TOTAL_UNRECOVERABLE_PKTS"
	let TOTAL_RECOVERABLE_PKTS=$STATS_ECC\&0xffff; echo "TOTAL_RECOVERABLE_PKTS:$TOTAL_RECOVERABLE_PKTS"
	
	set_csid_addr $1 0x94;
	CSID_STATS_CRC=`cat $phy_value_path`;echo "CSID_STATS_CRC:$CSID_STATS_CRC";
	
	set_csid_addr $1 0x60;
	CSID_IRQ_MASK=`cat $phy_value_path`;echo "CSID_IRQ_MASK : $CSID_IRQ_MASK";
	
	set_csid_addr $1 0x64;
	CSID_IRQ_STATUS=`cat $phy_value_path`;echo "CSID_IRQ_STATUS : $CSID_IRQ_STATUS";
	
}

function set_csid_irq_mask
{
	if(($#<1))
	then
		echo "need set which csid"
		return;
	fi
	set_csid_addr $1 0x60;
	echo $2 > $phy_value_path;
}

function print_csid_mem()
{
	if(($#<1))
	then
		echo "need set which csid"
		return;
	fi
	set_csid_addr $1 0x0; 
	echo $csid_reg_length > $count_path;
	cat $phy_printmems;
}

function print_csid
{
	if(($#<1))
	then
		echo "need set which csid"
		return;
	fi
	print_csid_status $1;
	print_csid_all $1;
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

CSID0_ECC_STAT_BASE=0xfda00090;
CSID0_CRC_STAT_BASE=0xfda00094;
function csid_ecc_crc()
{
	if(($#==1))
	then
	let ecc_stat_addr=$CSID0_ECC_STAT_BASE+$1*0x400;
	echo "ECC :";
	io_value $ecc_stat_addr;
	let crc_stat_addr=$CSID0_CRC_STAT_BASE+$1*0x400;
	echo "CRC :"
	io_value $crc_stat_addr;
	fi
}


function help_csid()
{
	echo "print_csid_status to print all status";
	echo "set_csid_irq_mask to set irq mask";
	echo "print_csid_mem to print all register";
	echo "print_csid to print all";
	echo "io_value read or write io value directly"
	echo "csid_ecc_crc (csid) to read the ecc and crc stat";
}




help_csid;