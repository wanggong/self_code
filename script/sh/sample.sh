function func_test()
{
a="wang";  #there is no space between 'a' and '=' and 'wang' 
echo "wang";
}
#warning there is no brace'()' after the function name;use the function name to call function directly
function func_usage()
{
	func_test;
}
#func parameters test
# $# is the count of the parameters , 
function func_param_test
{
	echo "long write length is $#";
	let index=1;
	while [ 1 ]
	do
		if(($index<=$#))
		then
			parameter=$(eval echo \$$index);
			echo "parameter:$parameter"
			let index=$index+1;
		else
			break;
		fi
	done
}
function func_param_usage()
{
	func_param_test wang li zhao zhang;
}

#input parameters , use $1,$2 ,...... to get it
#echo $1;
#echo $2;
#echo $3;

function if_else_then()
{
	if(($#<1))
	then
		echo "parameter is zero"
	elif(($#<2))
	then
		echo "parameter is one";
	fi
}

while_function()
{
	index=0;
	while [ 1 ]
	do
		if(($index<5))
		then
			echo wang
			index=$index+1;
		else
			break;
		fi
	done
}

#while function usage
function test_f()
{
	echo wang
}
#$1 times for while
#$2 the function to exec
function while_function_base()
{
	index=0;
	while [ 1 ]
	do
		if(($index<$1))
		then
			$2 $index
			index=$index+1;
		else
			break;
		fi
	done
}

function test_while_function_base()
{
	while_function_base 3 test_f
}

#array test
#${#array[*]} is the length of array
#${new_array[0]} is the array item 0
#${new_array[*] is the all array items 

array_usage()
{
  local new_array;
  new_array=(1 2 3 4 5 6 7);
  echo "original array is ${new_array[*]}";
  echo "length: ${#new_array[*]}";
  new_array[0]=90;
  echo ${new_array[0]};
  echo ${new_array[1]};
  echo ${new_array[2]};
  echo ${new_array[3]};
}

#array parameters
array_parameter_usege()
{
	local new_array;
	new_array=(`echo "$@"`)
  local array_len=${#new_array[*]}
  echo "total num is $array_len"
  echo ${new_array[0]}
  echo ${new_array[1]}
  echo ${new_array[2]}
}
array_parameter_usege_test()
{
	array1=(1 2 3 4 5);
	array_parameter_usege ${array1[*]}	
}

#${var/a/b} will use b replace a in var
#${var//a/b} will use b replace all a in var
string_replace_usege()
{
	local name=wanggongzhenwanggongzhenwanggongzhen;
	local rname1=${name/wang/abcd};
	echo $name;
	echo $rname1;
	local rname2=${name//wang/abcd};
	echo $name;
	echo $rname2;
	
}



new_array1=(
 {0x1e,0x13},
 {0x21,0x10},
 {0x15,0x16},
 {0x71,0x18},   
 {0x7c,0x18},  
 {0x76,0x19},
 )


address_start=0;
array_value_set()
{
	local new_array;
	new_array=(`echo "$@"`)
  local array_len=${#new_array[*]}
  echo "total num is $array_len"
  echo "start_address:$address_start"
  local index=0;
  
	while [ 1 ]
	do
		if(($index<$array_len))
		then
			local temp1=${new_array[$index]}
			temp1=${temp1/,/};
			index=$index+1;
			local temp2=${new_array[$index]}
			temp2=${temp2/,/};
			index=$index+1;
			echo $temp1;
			echo $temp2;
			let result=$temp1\<\<16\|$temp2;
			echo $result;
			
			
		else
			break;
		fi
	done
}

address_start=0x123456;
array_value_set ${new_array1[*]}


 //str cmp
function str_cmp()
{
raw="f08f5aa6106b1f348b9feed2ef299a25 /system/bin/sh"
md5sum="f08f5aa6106b1f348b9feed2ef299a25 /system/bin/sh"
if [[ $md5sum != $raw ]]
then
echo "not equal" 
echo $md5sum
echo $raw
else
echo "equal"
fi
}

function str_cmp2()
{
#md5sum="f08f5aa6106b1f348b9feed2ef299a25 /system/bin/sh"
if [[ $md5sum != f08f5aa6106b1f348b9feed2ef299a25* ]]
then
echo "not equal" 
echo $md5sum
echo $raw
else
echo "equal"
fi
}
 
 #string join
debugfs_dir="/data/debugfs/"
value_dir=${debugfs_dir}"/wanggongzhen"