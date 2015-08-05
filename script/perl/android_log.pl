#!/usr/bin/perl

#$_[0] is the line input
sub convert
{
	$linked_shard_library_dir="./lenovo/obj/SHARED_LIBRARIES";
	$linked_bin_dir="./lenovo/obj/EXECUTABLES/";
	$_ = $_[0];
	printf("%s" , $_);
	if(/#\d\d\s+pc ([0-9a-f]{8})\s+.*\/(.+)(\.so)?/)
	{
		$address=$1;
		$short_lib_name=$2;
		#printf($"%s\n" , $address);
		#printf("%s\n" , $short_lib_name);
		$find_full_lib_name_cmd="find $linked_shard_library_dir -name $short_lib_name.so";
		$full_lib_name=`$find_full_lib_name_cmd`;
		if($full_lib_name=="")
		{
			$find_full_lib_name_cmd="find $linked_bin_dir -name $short_lib_name|grep LINKED";
                	$full_lib_name=`$find_full_lib_name_cmd`;
		}
		#printf("%s\n" , $full_lib_name);
		$addr_to_line_cmd="arm-linux-androideabi-addr2line $address -a -i -p -s -f -C -e ./lenovo/obj/EXECUTABLES/adbd_intermediates/LINKED/adbd ";
		#printf("%s\n" , $addr_to_line_cmd);
		$line_func_file=`$addr_to_line_cmd`;
		printf("%s" , $line_func_file);

	}
	else
	{
		#printf("not match");
	}		
	
}

sub main()
{
	$_=$ARGV[0];
	if(/-i/)
	{
		convert($ARGV[1]);
	}
	else
	{
		$input_file=$ARGV[0];
		open(OPENFILE , $input_file);
		open(SAVEFILE ,">", "$input_file.log");
		$line1 = <OPENFILE>;
		$index=0;
		select SAVEFILE;
		while ($line1)
		{
			convert($line1);
		        $line1= <OPENFILE>;
		        
		}
		select STDOUT;
		close(OPENFILE);
		close(SAVEFILE);
	}
}
main();

