. base.sh
. gpio.sh
. pmic.sh


#phy_address=0xc10bf004
phy_address=`cat $suspend_resume_addr`;
#echo $phy_address;
let suspend_resume_count=1024*4;
let suspend_value_address=$phy_address+$suspend_resume_count;
let resume_value_address=$phy_address+$suspend_resume_count*2;
phy_index=0;



gpio_value_addr=0xFD511004;
GPIO_HIGH=2;
GPIO_LOW=0;
function set_gpio_suspend_resume()
{
	let address=$phy_address+$phy_index*4;
	get_gpio_value_addr $1;
	let gpio_phy_address=$get_gpio_value_addr_returned;
	vir_value $address $gpio_phy_address;
	
	
	let gpio_suspend_value_address=$suspend_value_address+$phy_index*4;
	vir_value $gpio_suspend_value_address $2;
	
	let gpio_resume_value_address=$resume_value_address+$phy_index*4;
	vir_value $gpio_resume_value_address $3;
	
	let phy_index=$phy_index+1;
	
}

function set_ldo_suspend_resume()
{
	get_ldo_enable_address $1;
	ldo_enable_address=$get_ldo_enable_address_returned;
	pmic_update_write_array $ldo_enable_address $2;

	let address=$phy_address+$phy_index*4;
	vir_value $address ${parameter_format_io_address_array[0]};

	let pmic_suspend_value_address=$suspend_value_address+$phy_index*4;
	vir_value $pmic_suspend_value_address $2;

	let pmic_resume_value_address=$resume_value_address+$phy_index*4;
	vir_value $pmic_resume_value_address $3;

	let phy_index=$phy_index+1;

	let command_address=$phy_address+$phy_index*4;
	vir_value $command_address ${parameter_format_io_address_array[1]};

	let pmic_command=${parameter_format_io_value_array[1]};
	let pmic_suspend_command_address=$suspend_value_address+$phy_index*4;
	vir_value $pmic_suspend_command_address $pmic_command;

	let pmic_resume_command_address=$resume_value_address+$phy_index*4;
	vir_value $pmic_resume_command_address $pmic_command;

	let phy_index=$phy_index+1;
	let parameter_format_io_count=0;
}


function reset_suspend_resume()
{
	local index=0;
	while [ 1 ]
	do
		if(($phy_index>=0))
		then
			let address=$phy_address+$phy_index*4;
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
		mask_value=`cat $suspend_resume_debug_path`;
		echo "value:$mask_value";
	else
		echo $1 > $suspend_resume_debug_path;
	fi
}




function base_help()
{
	echo "----------------------------SUSPEND_RESUME----------------------------------------------";
	echo "set_gpio_suspend_resume gpio_num suspend_value resume_value (0 LOW, 2 HIGH)"
	echo "set_ldo_suspend_resume ldo_num suspend_value resume_value (0 disable 0x80 enable)"
	echo "reset_suspend_resume reset it"
	echo "suspend_resume_test value (0 for suspend , 1 for resume)"
#suspend_resume_debug
#bit[0]		dupm all ldo register[resume]
#bit[1]		show ldo consumers[resume]
#bit[2]		dump gpio registers[resume]
#bit[3]		show all gpios info[resume]
#bit[4]		dupm all ldo register[suspend]
#bit[5]		show ldo consumers[suspend]
#bit[6]		dump gpio registers[suspend]
#bit[7]		show all gpios info[suspend]
#bit[30]	exec resume set
#bit[31]	exec suspend set
	echo "suspend_resume_debug_mask (-value)"
	echo "----------------------------------------------------------------------------------------";
}

base_help
