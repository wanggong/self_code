#!/usr/bin/perl
sub main()
{
        open(OPENFILE , "./9806E+BOE4.5.txt");
        open(SAVEFILE ,">", "./out/out.txt");
	open(LKFILE,">","./out/lk.txt");
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
               
               if($count>=1)
               {
			printf(SAVEFILE "{%s , %d , {",$filds[0] , $count);
	                for($i=1;$i<=$count;++$i)
	                {
	                	$_=$filds[$i];
	                	printf(SAVEFILE "%s" , $_);
				if($i!=$count)
				{
				    printf(SAVEFILE ",");
				}
	                }
					
	                printf(SAVEFILE "}},\n");
		
              	}
                $line1= <OPENFILE>;
               
        }
        close(OPENFILE);
        close(SAVEFILE);
		close(LKFILE);
}
main();

