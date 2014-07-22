#!/usr/bin/perl
sub main()
{
		$linked_shard_library_dir="/home/wanggongzhen/project/8916/out/target/product/msm8916_32_512/obj/SHARED_LIBRARIES";
		$input_file=$ARGV[0];
        open(OPENFILE , $input_file);
        open(SAVEFILE ,">", "$input_file.log");
        $line1 = <OPENFILE>;
		$index=0;
        while ($line1)
        {
                $_ = $line1;
				printf(SAVEFILE "%s" , $line1);
				if(/#\d\d  pc ([0-9a-f]{8})  .*\/(.+)\.so/)
				{
					printf(SAVEFILE "%s" , $_);
					$address=$1;
					$short_lib_name=$2;
					#printf(SAVEFILE "%s\n" , $address);
					#printf(SAVEFILE "%s\n" , $short_lib_name);
					$find_full_lib_name_cmd="find $linked_shard_library_dir -name $short_lib_name.so";
					$full_lib_name=`$find_full_lib_name_cmd`;
					#printf(SAVEFILE "%s\n" , $full_lib_name);
					$addr_to_line_cmd="arm-linux-androideabi-addr2line $address -a -i -p -s -f -C -e $full_lib_name ";
					#printf(SAVEFILE "%s\n" , $addr_to_line);
					$line_func_file=`$addr_to_line_cmd`;
					printf(SAVEFILE "%s\n" , $line_func_file);
					
				}
                $line1= <OPENFILE>;
                
        }
        close(OPENFILE);
        close(SAVEFILE);
		close(LKFILE);
}
main();

