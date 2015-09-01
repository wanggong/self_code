
. base.sh;
. gpio.sh;
#. pmic.sh;



#phy_address=0xc10bf004
#phy_address=`cat $suspend_resume_addr`;
read_value $suspend_resume_addr;
#echo $read_value_returned;
let phy_address=$read_value_returned;
#echo $phy_address;
let suspend_resume_count=1024*8;
#let suspend_value_address=($phy_address+$suspend_resume_count);
#let resume_value_address=($phy_address+$suspend_resume_count*2);
phy_index=0;



#gpio_value_addr=0xFD511004;
#GPIO_HIGH=2;
#GPIO_LOW=0;
function suspend_resume_init()
{
	echo "suspend_resume_init start"
	let phy_index=0;
	while [ 1 ]
	do
		let address=$phy_address+$phy_index*8;
		vir_value_get $address;
		echo $vir_value_get_returned;
		if(($vir_value_get_returned>0))
		then	
			let phy_index=$phy_index+1;
		else
			break;
		fi
	done
	#echo "after suspend_resume_init phy_index $phy_index";
}

function set_io_value_suspend_resume()
{
	echo "set $1 $2 $3"
	let address=$phy_address+$phy_index*8;
	vir_value $address $1;
	
	let io_suspend_value_address=$suspend_value_address+$phy_index*8;
	vir_value $io_suspend_value_address $2;
	
	let io_resume_value_address=$resume_value_address+$phy_index*8;
	vir_value $io_resume_value_address $3;
	
	let phy_index=$phy_index+1;
	
}


function set_gpio_suspend_resume()
{
	get_gpio_value_addr $1;
	let gpio_phy_address=$get_gpio_value_addr_returned;
	set_io_value_suspend_resume $gpio_phy_address $2 $3;
}

function set_pimc_reg_suspend_resume()
{

#write suspend resume value
	pmic_update_write_array $1 $2;
	set_io_value_suspend_resume ${parameter_format_io_address_array[0]} $2 $3
#write command 
	let pmic_command=${parameter_format_io_value_array[1]};
	set_io_value_suspend_resume ${parameter_format_io_address_array[1]} $pmic_command $pmic_command

	let parameter_format_io_count=0;
}

function set_ldo_suspend_resume()
{
	get_ldo_enable_address $1;
	ldo_enable_address=$get_ldo_enable_address_returned;
	set_pimc_reg_suspend_resume $ldo_enable_address $2 $3;
}


function reset_suspend_resume()
{
	local index=0;
	while [ 1 ]
	do
		if(($phy_index>=0))
		then
			let address=$phy_address+$phy_index*8;
			vir_value $address 0;
			let phy_index=$phy_index-1;
		else
			break;
		fi
	done
	let phy_index=$phy_index+1;
}

function suspend_resume_test()
{
	echo $1 > $suspend_resume_test;
}

function suspend_resume_debug_mask()
{
	if(($#<1))
	then
		#mask_value=`cat $suspend_resume_debug_path`;
		read_value $suspend_resume_debug_path;
		echo "value:$read_value_returned";
	else
		#echo $1 > $suspend_resume_debug_path;
		write_value $1 $suspend_resume_debug_path;
	fi
}

function disable_wakeup_irk()
{
	if(($#<1))
	then
		read_value $gic_wakeup_irqs;
		echo "addr:"$read_value_returned;
		let address=$read_value_returned;
		index=0;
		while [ 1 ]
		do
			if(($index<32))
			then
				vir_value_get $address;
				echo $vir_value_get_returned;
			else
				break;
			fi
			index=$index+1;
			let address=$address+4;
		done
	else
		let irqindex=$1;
		let to32_addr=$irqindex\>\>5;
		let index=$irqindex%32;
		read_value $gic_wakeup_irqs;
		#echo "addr:"$read_value_returned;
		let address=$read_value_returned+$to32_addr*4;
		#echo "addr12:"$address
		vir_value_get $address;
		#echo "re:$vir_value_get_returned:$index"
		let wr_value=$vir_value_get_returned\|1\<\<$index;
		#echo "aaaa $address:$wr_value"
		vir_value $address $wr_value;
	fi
}



function base_help()
{	
	echo "----------------------------SUSPEND_RESUME----------------------------------------------";
	echo "set_io_value_suspend_resume io_address suspend_value resume_value"
	echo "set_gpio_suspend_resume gpio_num suspend_value resume_value (0 LOW, 2 HIGH)"
	echo "set_pimc_reg_suspend_resume ldo_num suspend_value resume_value"
	echo "set_ldo_suspend_resume ldo_num suspend_value resume_value (0 disable 0x80 enable)"
	echo "reset_suspend_resume reset it"
	echo "suspend_resume_test value (0 for suspend , 1 for resume)"
	echo "disable_wakeup_irk irq"
#suspend_resume_debug
#bit[0]		dupm all ldo status
#bit[1]		dupm gpio ldo status
#bit[2]		dump interrupt registers
#bit[31]	exec command set
	echo "suspend_resume_debug_mask (-value)"
	echo "----------------------------------------------------------------------------------------";
}
#suspend_resume_init
base_help
