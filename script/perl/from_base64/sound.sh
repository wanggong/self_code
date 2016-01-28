. base.sh
function set_sound_addr()
{
	echo $1 > $sound_register_address_path; 
}

sound_value_get_returned=-1;
function sound_value_get()
{
	set_sound_addr $1;
	sound_value_get_returned=`cat $sound_register_value_path`;
}

function sound_value_set()
{
	set_sound_addr $1;
	#echo "value : $2";
	echo $2 > $sound_register_value_path;
}


function sound_value()
{
	let address=$1;
	if(($#<2))
	then
		sound_value_get $address;
		echo "value : $sound_value_get_returned";
	else
		sound_value_set $address $2;
	fi
}



function sound_help()
{
	echo "------------------SOUND RESGISTER --------------------";
	echo "sound_value address (-value)";
	echo "--------------------------------------------";
}

sound_help




