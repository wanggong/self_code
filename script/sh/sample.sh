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
	echo "parameter count is $#" ;
#	for((i=0;$i<$#;i++))
	{
		eval b=\$$i;	
		echo $b;
	}
}
function func_param_usage()
{
	func_param_test wang li zhao zhang;
}

#input parameters , use $1,$2 ,...... to get it
#echo $1;
#echo $2l
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