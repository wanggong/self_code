CCI_ADDR_TYPE_PATH="/d/cci_info/addr_type";
CCI_DATA_TYPE_PATH="/d/cci_info/data_type";
CCI_I2c_ADDR_PATH="/d/cci_info/i2c_addr";
CCI_REG_ADDR_PATH="/d/cci_info/reg_addr";
CI_VALUE_PATH="/d/cci_info/value";

function cci_config()
{
	echo $1 > $CCI_I2c_ADDR_PATH;
	echo $2 > $CCI_ADDR_TYPE_PATH;
	echo $3 > $CCI_DATA_TYPE_PATH;
}

function cci_value()
{
	echo $1 > $CCI_REG_ADDR_PATH;
	if(($#<2))
	then
		cci_value_returned=`cat $CI_VALUE_PATH`;
		echo "cci_value:$cci_value_returned";
	else
		echo $2 > $CI_VALUE_PATH;
	fi
}


function help_cci()
{
	echo "----------------------CCI--------------------------------";
	echo "cci_config i2c_addr(right shift 1) addr_type(1 for byte 2 for WORD) data_type(1 for byte 2 for WORD) ";
	echo "cci_value reg_addr (-value) read or write i2c register";
	echo "----------------------------------------------------------";
}
help_cci;




