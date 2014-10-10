function dump_base()
{
	let frm_num=0xffff0000;
	let dump_type=$1;
	let value=$frm_num\|$dump_type;
	echo $value;
	setprop persist.camera.dumpimg $value;
}

function dump_preview()
{
	dump_base 1;
}
