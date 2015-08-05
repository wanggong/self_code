#!/usr/bin/perl
$red=0;
$green=0;
$blue=0;
$index=0;
$step=140;
sub update_color()
{
	if($index%7==0||$index%7==3)
	{
		$red+=$step;
		$red=$red%255;
	}
	elsif($index%7==1||$index%7==4)
	{
		$green+=$step;
		$green=$green%255;
		if($red>$step)
		{
			$red-=$step;
		}
		else
		{
			$red+=(255-$step);
			$red=$red%255;
		}
	}
	elsif($index%7==2)
	{
		$blue+=$step;
		$blue=$blue%255;
		if($green>$step)
		{
			$green-=$step;
		}
		else
		{
			$green+=(255-$step);
			$green=$green%255;
		}
	}
	elsif($index%7==5)
	{
		$red+=$step;
		$red=$red%255;
		if($blue>$step)
		{
			$blue-=$step;
		}
		else
		{
			$blue+=(255-$step);
			$blue=$blue%255;
		}
	}
	elsif($index%7==6)
	{
		$blue+=$step;
		$blue=$blue%255;
	}
	
	$index++;
}

@type_array = 
(
"input"
,"output"
,"mux"
,"virt_mux"
,"value_mux"
,"mixer"
,"named_ctl"
,"pga"
,"out_drv"
,"adc"
,"dac"
,"micbias"
,"mic"
,"hp"
,"spk"
,"line"
,"switch"
,"vmid"
,"pre"
,"post"
,"supply"
,"regulator_supply"
,"clock_supply"
,"aif_in"
,"aif_out"
,"siggen"
,"dai_in"
,"dai_out"
,"dai_link"
);

sub main()
{

	$input_file=$ARGV[0];
    open(OPENFILE , $input_file);
    open(SAVEFILE ,">", "$input_file.gv");
    $line1 = <OPENFILE>;
	$widget="";
	$widget_id="";

	printf(SAVEFILE "digraph a{\n");
    while ($line1)
    {
        $_ = $line1;
		#printf(SAVEFILE "%s" , $line1);
		if(/widget,name:(.*),sname:.*,con:(\d*),active:(\d*),id:(\d*),powered:(\d*),input:(\d*),output:(\d*),num_kcontrols=(\d*)/)
		{
			#printf(SAVEFILE "%s" , $_);
			$widget=$1;
			#printf(SAVEFILE "%s\n" , $widget);
			$widget_id=$4;
			$powered=$5;
			#printf(SAVEFILE '"%s"[label="%s(%s,C:%d,A:%d,ID:%d,P:%d,I:%d,O:%d,N:%d)"]',$widget,$widget,$type_array[$widget_id],$2,$3,$4,$5,$6,$7,$8);
			if($powered)
			{
				printf(SAVEFILE '"%s"[label="%s(%s)"]',$widget,$widget,$type_array[$widget_id]);
			}
			printf(SAVEFILE "\n");
		}
		if(/source,name:(.*),sname:.*,con:(\d*),active:(\d*),id:(\d*),powered:(\d*),input:(\d*),output:(\d*),num_kcontrols=(\d*)/)
		{
			update_color();
			$source=$1;
			$widget_id=$4;
			$powered=$5;
			if($powered)
			{
				printf(SAVEFILE '"%s"[label="%s(%s)"]',$source,$source,$type_array[$widget_id]);
			}
			printf(SAVEFILE "\n");
			printf(SAVEFILE '"%s" -> "%s"' , $source , ,$widget);
			if(!$powered)
			{
				printf(SAVEFILE '[style=dotted]');
				printf(SAVEFILE '[color="#%2x%2x%2x"]' ,$red, $green,$blue);
			}
			else
			{
				printf(SAVEFILE '[color="#FF0000"]');
			}
			printf(SAVEFILE "\n");
		}
        $line1= <OPENFILE>;    
    }
	printf(SAVEFILE "}\n");
	close(OPENFILE);
	close(SAVEFILE);
}
main();
