
sub compile_files
{
	my $folder=$_[0];
	#printf("folder=%s\n",$folder);
	my $compile_cmd=sprintf("gcc ./%s/*.c -o %s" , $folder , $folder);
	#printf("compile_cmd=%s\n",$compile_cmd);
	system($compile_cmd);
}

sub execute_script
{
	my $folder=$_[0];
	my $command=sprintf(".//%s.exe" , $folder);
	#printf("command=%s\n",$command);
	system($command);
}

sub main
{
	my $folder=$ARGV[0];
	#printf("folder=%s\n",$folder);
	compile_files $folder;
	execute_script $folder;
}

main;

