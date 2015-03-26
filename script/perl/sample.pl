#!/usr/bin/perl

#array define use like this
sub arraytest()
{
	@array1 = (1,2,2,3,4);
	printf("array[0]=%d\n", $array1[0]);
	printf("array length is %d\n" , $#array1);
	$array1[0] = 100;
	printf("array[0]=%d\n", $array1[0]);
}


sub vartest()
{
	$var = 9;
	$var2 = 90;
	printf($var);
	printf($var2);
}


#$_[0] is the first parameter, $_[1] is the second parameter, and $#_ is the parameters's count
sub functest
{
	my $i;
	my $result;
	printf("parameters count is %d\n" , $#_);
	for($i = 0 ; $i <= $#_ ; $i++)
	{
		printf("the %d parameters is %s\n",$i,$_[$i]);
		$result = $result + $_[$i];
	}
	return $result;
}
sub functest2
{
	functest(1,23,4,5,6);
}


#the input parameters is in @ARGV , such perl test.pl 1 2 5 3
#there will be have 4 parameters ,but $#ARGV = 3 (warning)
sub inputparameterstest
{
	my $i;
	my $result;
	printf("parameters count is %d\n" , $#ARGV);
	for($i = 0 ; $i <= $#ARGV ; $i++)
	{
		printf("the %d input parameters is %s\n",$i,$ARGV[$i]);
		$result = $result + $ARGV[$i];
	}
	print $result;
}

#split use
sub split_test
{
	$str = "wang,li,zhao,zhang";
	@str_arr = split(/,/,$str);
	for($i = 0 ; $i <= $#str_arr ; $i++)
	{
		printf("the %d is %s\n",$i,$str_arr[$i]);
	}
}

#join use
sub join_test
{
	$str1 = "hello";
	$str2 = "world";
	$join_str = sprintf("%s %s" , $str1 , $str2);
	printf("%s" , $join_str);
}

#open read write and close file test
#> is write to a file (will clean the file)
#>> is append to the file
sub filetest
{
	open(OPENFILE , "./test.txt");
	open(SAVEFILE ,">", "./out.txt");
	open(APPENDFILE ,">>", "./append.txt");
	$a = <OPENFILE>;#read a line
	while ($a)
	{
		printf(SAVEFILE "%s" , $a);#warning after SAVEFILE, there is no ","
		printf(APPENDFILE "%s" , $a);
		$a = <OPENFILE>;#read a line
	}
	close(OPENFILE);
	close(SAVEFILE);
	close(APPENDFILE);
}

#replace 
#replace is like this s/parrten/string/
#end with a g is replace all
#wanning the string will be replace and after replaced is use $_


sub replacestr()
{
	$str = "wanggongzhen wang wang";
	$_ = $str;
	printf("before repace %s\n" , $_);
	s/wang/abcd/;
	printf("after repace %s\n" , $_);

	$_ = $str;
	printf("before repace %s\n" , $_);
	s/wang/abcd/g;
	printf("after repace with g %s\n" , $_);
}
replacestr();

#for loop
sub forloop()
{
	$i = 3;
  for($i=3;$i<=100;$i++)
  {
  	printf("i is %d\n" , $i);
  	if(m/$ARGV[$i]/)
  	{
  		next;
  	}
  	else
  	{
  		last;
  	}
  }
  printf("$i = %d\n" , $i);
}
