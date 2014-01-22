
vir_value_path="/data/debugfs/debug_mem/value";
vir_addr_path="/data/debugfs/debug_mem/address";
phy_value_path="/data/debugfs/debug_mem/phy_value";
phy_addr_path="/data/debugfs/debug_mem/phy_addr";
count_path="/data/debugfs/debug_mem/count";
phy_printmems="/data/debugfs/debug_mem/phy_printmems";
vir_2_phy="/data/debugfs/debug_mem/vir_2_phy";
phy_2_vir="/data/debugfs/debug_mem/phy_2_vir";
suspend_resume_addr="/data/debugfs/debug_mem/suspend_resume_addr";

function set_value_addr()
{
	echo $1 > $vir_addr_path; 
}

function get_value()
{
	set_value_addr $1;
	value=`cat $vir_value_path`;echo "value : $value";
}

function set_value()
{
	set_value_addr $1;
	#echo "value : $2";
	echo $2 > $vir_value_path;
}


function vir_value()
{
	let address=$1;
	if(($#<2))
	then
		get_value $address;
	else
		set_value $address $2;
	fi
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

function vir_to_phy()
{
	echo $1 > $vir_2_phy;
	phy_value=`cat $vir_2_phy`;
	echo $phy_value;
}

function phy_to_vir()
{
	echo $1 > $phy_2_vir;
	vir_value=`cat $phy_2_vir`;
	echo $vir_value;
}

#phy_address=0xc10bf004
phy_address=`cat $suspend_resume_addr`;
echo $phy_address;
let suspend_resume_count=1024;
let suspend_value_address=$phy_address+$suspend_resume_count;
let resume_value_address=$phy_address+$suspend_resume_count*2;
phy_index=0;



gpio_value_addr=0xFD511004;
GPIO_HIGH=2;
GPIO_LOW=0;
function set_gpio_suspend_resume()
{
	let address=$phy_address+$phy_index*4;
	let gpio_phy_address=$gpio_value_addr+$1*0x10;
	vir_value $address $gpio_phy_address;
	
	
	let gpio_suspend_value_address=$suspend_value_address+$phy_index*4;
	vir_value $gpio_suspend_value_address $2;
	
	let gpio_resume_value_address=$resume_value_address+$phy_index*4;
	vir_value $gpio_resume_value_address $3;
	
	let phy_index=$phy_index+1;
	
}


pmic_data_addr=0xfc4cf810;
pmic_command_addr=0xfc4cf800;

let LDO_BASE_START=0x14000;
ONE_LDO_REGS=0x100;
ENABLE_REG_ADDR=0x46;
MODE_REG_ADDR=0x45;

function set_ldo_suspend_resume()
{
	  let address=$phy_address+$phy_index*4;
	  vir_value $address $pmic_data_addr;
	
	  let pmic_suspend_value_address=$suspend_value_address+$phy_index*4;
	  vir_value $pmic_suspend_value_address $2;
	
		let pmic_resume_value_address=$resume_value_address+$phy_index*4;
		vir_value $pmic_resume_value_address $3;
		
		let phy_index=$phy_index+1;
		
		let command_address=$phy_address+$phy_index*4;
	  vir_value $command_address $pmic_command_addr;
	  
	  let ldo_index=$1-1;
	  let pmic_command=\($LDO_BASE_START+$ldo_index*0x100+$ENABLE_REG_ADDR\)*0x10;
	  let pmic_suspend_command_address=$suspend_value_address+$phy_index*4;
	  vir_value $pmic_suspend_command_address $pmic_command;
	  
	  let pmic_resume_command_address=$resume_value_address+$phy_index*4;
	  vir_value $pmic_resume_command_address $pmic_command;
	   
		let phy_index=$phy_index+1;
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


function base_help()
{
	echo "vir_value address (-value)";
	echo "io_value address (-value)";
	echo "vir_to_phy value (convert virtual to phy)"
	echo "phy_to_vir value (convert phy to virtual)"
	echo "set_gpio_suspend_resume gpio_num suspend_value resume_value (0 LOW, 2 HIGH)"
	echo "set_ldo_suspend_resume ldo_num suspend_value resume_value (0 disable 0x80 enable)"
	echo "reset_suspend_resume reset it"
}

base_help
