#!/usr/bin/perl
sub main()
{
        open(OPENFILE , "./9605.txt");
        open(SAVEFILE ,">", "z:/out.txt");
		open(LKFILE,">","z:/lk.txt");
        $line1 = <OPENFILE>;
		$index=0;
        while ($line1)
        {
                $_ = $line1;
                $sub_start=index($_,"(");
                $sub_end=index($_,")");
                $_=substr($_,$sub_start+1,$sub_end-$sub_start-1);
                #printf(SAVEFILE "%s\n" ,  $_);
                @filds = split(/,/,$_);
                $count=$#filds;
                #printf(SAVEFILE "count:%d\n",$count);
                if($count==1)
                {
                	printf(SAVEFILE "\t\t\t\t\t 23 01 00 00 00 00 %.2x\n" , $count+1);
                }
                elsif($count>1)
                {
                	printf(SAVEFILE "\t\t\t\t\t 29 01 00 00 00 00 %.2x\n" , $count+1);
                }
               
               if($count>=1)
               {
			printf(SAVEFILE "\t\t\t\t\t\t");
	                for($i=0;$i<=$count;++$i)
	                {
	                	$_=$filds[$i];
	                	s/0x|0X//;
	                	printf(SAVEFILE "%s " , $_);
	                }
					
	                printf(SAVEFILE "\n");
					
					
			printf(LKFILE "static char dispon%d[] = \n{\n" , $index++);
			printf(LKFILE "\t0x%.2x,0x00,0x29,0xC0," , $count+1);
			for($i=0;$i<=$count;++$i)
	                {
				if($i%4==0)
				{
					printf(LKFILE "\n\t" );
				}
				printf(LKFILE "%s," , $filds[$i]);
		
				
						
	                }
			while($i%4!=0)
			{
				printf(LKFILE "0xFF," );
				$i++;
			}
			printf(LKFILE "\n};\n");
					
              	}
                $line1= <OPENFILE>;
               
        }
        close(OPENFILE);
        close(SAVEFILE);
		close(LKFILE);
}
main();

