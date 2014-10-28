#!/usr/bin/perl

sub main()
{
	$vmlinux="/home/wanggongzhen/projects/lenovo/android/out/target/product/wt86518_A116G/obj/KERNEL_OBJ/vmlinux";
	$input_file=$ARGV[0];
        open(OPENFILE , $input_file);
        open(SAVEFILE ,">", "$input_file.log");
        $line1 = <OPENFILE>;
		$index=0;
        while ($line1)
        {
                $_ = $line1;
		printf(SAVEFILE "%s" , $line1);
		if(/from \[\<([0-9a-f]{8})\>\]/)
		{
			#printf(SAVEFILE "%s" , $_);
			$address=$1;
			#printf(SAVEFILE "%s\n" , $address);
			$addr_to_line_cmd="arm-linux-androideabi-addr2line $address -a -i -p -s -f -C -e $vmlinux ";
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

