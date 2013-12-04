#!/usr/bin/perl
sub main()
{
        open(OPENFILE , "z:\System.map");
        open(SAVEFILE ,">", "z:/out.txt");
        $line1 = <OPENFILE>;
		$index=0;
        while ($line1)
        {
			$_ = $line1;
			@fields = split(/ /,$_);
			$count=$#fields;
			printf(SAVEFILE "count:%d\n",$count);
			if($count==2)
			{
				$address=sprintf("0x%s" , $fields[0]);
				$name=$fields[2];
				printf(SAVEFILE "$fields = %s , %s" , $address , $name);
			}
			$line1 = <OPENFILE>;

        }
        close(OPENFILE);
        close(SAVEFILE);
}
main();

