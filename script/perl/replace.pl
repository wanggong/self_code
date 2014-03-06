#!/usr/bin/perl
sub help()
{
		printf("perl replace.pl repalce_file_name will_be_replace replaced (must_matchs...)\n");
}
sub main()
{
				if($#ARGV < 2)
				{
					help();
					return;
				}
				$file_name = $ARGV[0];
				$will_replace = $ARGV[1];
				$replaced = $ARGV[2];
				
				$replaced_file_name = sprintf("%s_%s",$file_name,"out");
	
        open(OPENFILE , $file_name);
        open(SAVEFILE ,">", $replaced_file_name);
        
        $line1 = <OPENFILE>;
        while ($line1)
        {
                $_ = $line1;
                $i = 3;
                for($i=3;$i<=$#ARGV;$i++)
                {
                	if(m/$ARGV[$i]/)
                	{
                		next;
                	}
                	else
                	{
                		last;
                	}
                }
               if($i > $#ARGV)
               {
               		s/$will_replace/$replaced/;
               }
               printf(SAVEFILE "%s" , $_);
               $line1= <OPENFILE>;
        }
        close(OPENFILE);
        close(SAVEFILE);
        
        $file_back = sprintf("%s_%s" , $file_name , "bak");
 
        rename($file_name,$file_back);
        rename($replaced_file_name,$file_name);
}
main();

