
sub compile_files
{
	my $folder=$_[0];
	#printf("folder=%s\n",$folder);
	my $compile_cmd=sprintf("gcc ./%s/*.c -o ./%s/%s" , $folder , $folder, $folder);
	#printf("compile_cmd=%s\n",$compile_cmd);
	$err = system($compile_cmd);
	if($err != 0)
	{
		die("compile error");
	}
}

sub execute_script
{
	my $folder=$_[0];
	my $args=$_[1];
	my $command=sprintf(".//%s.exe %s" , $folder , $args);
	#printf("command=%s\n",$command);
	$err = system($command);
	if($err != 0)
	{
		die("exec cmd %s error" , $command);
	}
	else
	{
		printf("%s exec success\n" , $command);
	}
}

sub main
{
	my $folder=$ARGV[0];
	#printf("folder=%s\n",$folder);
	compile_files $folder;
	
	my $i;
	my $args;
	for($i = 1 ; $i <= $#ARGV ; $i++)
	{
		$args = sprintf("%s %s ",$args,$ARGV[$i]);
	}	
	execute_script($folder,$args);
}

main;

