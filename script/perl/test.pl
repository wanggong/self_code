#!/usr/bin/perl
sub main()
{
        open(OPENFILE , "./test.txt");
        open(SAVEFILE ,">", "./out.txt");
        $line1 = <OPENFILE>;
        while ($line1)
        {
                $_ = $line1;
                s/\(|\)|,|;/ /g;
                s/\s+/ /g;
                @filds = split(/ /,$_);
                $next_line_count = $filds[$#filds];
                printf("next count is %d\n" , $next_line_count);
                $temp = $_;
                $_ = <OPENFILE>;
                $temp = sprintf("%s %s" , $temp,$_);
                printf($temp);print "\n";
                $line1= <OPENFILE>;
        }
        close(OPENFILE);
        close(SAVEFILE);
}
main();

