#!/usr/bin/perl	
$ab=0xffffffc001230360;
$ac=$ab+100;
print $ab;
print "\n";
print $ac;

sub test1()
{
print("%s",$_[0]);

}

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

inputparameterstest();
