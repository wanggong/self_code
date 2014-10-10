function dump_pic()
{
	let frm_num=0xffff0000;
	let dump_type=$1;
	let value=$frm_num\|$dump_type;
	#echo $value;
	setprop persist.camera.dumpimg $value;
}

function dump_zsl_raw()
{
	if(($1==1))
	then
		setprop persist.camera.zsl_raw 1;
		dump_pic 16;
	else
		setprop persist.camera.zsl_raw 0;
		dump_pic 0;
	fi
}

function dump_zsl_yuv()
{
	if(($1==1))
	then
		setprop persist.camera.zsl_yuv 1;
		dump_pic 4;
	else
		setprop persist.camera.zsl_raw 0;
		dump_pic 0;
	fi
}


function dump_metadata()
{
	setprop persist.camera.dumpmetadata 1;
}

function dump_nonzsl_yuv()
{
	if(($1==1))
	then
		setprop persist.camera.nonzsl.yuv 1;
		dump_pic 4;
	else
		setprop persist.camera.nonzsl.yuv 0;
		dump_pic 0;
	fi
}

function dump_preview_raw()
{
	if(($1==1))
	then
		setprop persist.camera.preview_raw 1;
		dump_pic 16;
	else
		setprop persist.camera.preview_raw 0;
		dump_pic 0;
	fi
}

function dump_snapshot_raw()
{
	if(($1==1))
	then
		setprop persist.camera.snapshot_raw 1;
		dump_pic 16;
	else
		setprop persist.camera.snapshot_raw 0;
		dump_pic 0;
	fi
}

function set_hal_debug_mask()
{
	let mask=$1;
	setprop persist.camera.hal.debug.mask $mask;
}


function help()
{
	echo "------------------common dump-----------------------------------"
	echo "dump_pic 0 STOP";
	echo "dump_pic 1 PREVIEW ---for preview(in preview_stream_cb_routine and nodisplay_preview_stream_cb_routine)";
	echo "dump_pic 2 VIDEO   ---for video(in video_stream_cb_routine)";
	echo "dump_pic 4 SNAPSHOT"
	echo "dump_pic 8 THUMBNAIL"
	echo "dump_pic 16 RAW"
	echo "dump_pic 32 JPEG"
	echo "setprop persist.debug.sf.showfps 1 (1 show fps , 0 disable)"
	echo "dump_pic 32 (dump jpeg to file , 32:enable 0:disable)"
	echo "---------------------------------------------------------------------"

	echo "-----------------in zsl_channel_cb------------------------------"
	echo "dump_zsl_raw 1(1 enable , 0 disable)"
	echo "dump_zsl_yuv 1(1 enable , 0 disable)"
	echo "setprop persist.camera.dumpmetadata 1 (1 for dump , 0 disable)"
	echo "setprop persist.camera.zsl_matching 1(1 enable 0 disable)"
	echo "----------------------------------------------------------------";
	
	echo "-----------------in capture_channel_cb_routine------------------------------"
	echo "dump_nonzsl_yuv 1(1 enable , 0 disable)"
	echo "setprop persist.camera.dumpmetadata 1 (1 for dump , 0 disable)"
	echo "----------------------------------------------------------------";

	echo "-----------------in snapshot_channel_cb_routine------------------------------"
	echo "setprop persist.camera.dumpmetadata 1 (1 for dump , 0 disable)"
	echo "----------------------------------------------------------------";

	echo "-----------------in preview_raw_stream_cb_routine------------------------------"
	echo "dump_preview_raw 1(1 enable , 0 disable)"
	echo "----------------------------------------------------------------";

	echo "-----------------in snapshot_raw_stream_cb_routine------------------------------"
	echo "dump_snapshot_raw 1(1 enable , 0 disable)"
	echo "----------------------------------------------------------------";

	echo "-----------------other(in QCamera2HWI.cpp)-------------------------------------------"
	echo "setprop persist.camera.4k2k.enable 1		(1:enable 0:disable default:0)"
	echo "setprop persist.camera.mem.usepool 1		(1:enable 0:disable defualt:1)"
	echo "setprop persist.camera.mem.usecache 1		(1:enable 0:disable defualt:1)"
	echo "setprop persist.camera.is_type 1     		(1:enable 0:disable defualt:0)"
	echo "setprop persist.camera.raw_yuv 1     		(1:enable 0:disable defualt:0)"
	echo "setprop persist.camera.feature.cac 1 		(1:enable 0:disable defualt:0)"
	echo "setprop persist.camera.feature.restart 		1(1:enable 0:disable defualt:0)"
	echo "setprop persist.camera.feature.shutter 1		(1:enable 0:disable defualt:0)"
	echo "setprop set_hal_debug_mask 0x10000007		(0x30000007 for all , 0 for none defalut:0x10000007)"

	echo "-----------------other(QCameraParameters.cpp)-------------------------------------------"
	echo "setprop setprop persist.debug.sf.showfps 1	(1:enable 0:disable default:0)"
	echo "setprop persist.camera.thermal.mode frameskip	(frameskip or fps default:frameskip)"
	echo "setprop persist.camera.opt.livepic 1		(1:enable 0:disable defualt:1)"
	echo "setprop persist.camera.stats.opt.mask 1     	(1|2|4|8|16 defualt:0)"
	echo "setprop persist.camera.stats.debug.mask 1     	()"
	echo "setprop persist.camera.ISP.debug.mask 1 		()"
	echo "setprop persist.camera.auto.hdr.enable enable 	(enable or disable)"
	echo "setprop persist.capture.burst.exposures ""	(defualt:"")"
	echo "setprop persist.camera.zsl.interval 1		(default:1)"
	echo "setprop persist.camera.zsl.backlookcnt 2		(defualt:2)"
	echo "setprop persist.camera.zsl.queuedepth 2		(default:2)"
	echo "setprop persist.camera.snapshot.number 0		(defualt:0)"
	echo "setprop persist.camera.snapshot.fd 0		(default:0)"
	echo "setprop persist.camera.mobicat 0			(default:0)"
	echo "setprop persist.camera.hdr.outcrop disable	(enable or disable)"
	echo "setprop persist.debug.set.fixedfps 0		(default:0)"
	echo "setprop persist.camera.continuous.iso 0		(default:0)"
	echo "setprop persist.camera.tintless enable		(enable or disable default:enable)"
	echo "setprop persist.camera.CDS on			(on or off default:on)"
	echo "setprop persist.camera.set.afd 3			(default:3)"
	echo "setprop persist.denoise.process.plates ""		(0,1,2,3 default:"")"
	echo "persist.camera.snap.format 0			(0 or 1 default:0)"
	echo "persist.camera.raw.format 16			(default:16)"

	echo "-----------------other(QCameraPostProc.cpp)-------------------------------------------"
	echo "setprop persist.camera.longshot.save 0		(0 or 1 default:0)"
	echo "setprop persist.camera.jpeg_burst 0		(default:0)"


	echo "-----------------other(pproc_module.c)-------------------------------------------"
	echo "setprop persist.camera.pproc.debug.mask 268435463	(default:268435463)"

	echo "-----------------other(module_ubifocus.c)-------------------------------------------"
	echo "setprop persist.camera.imglib.refocus 0		(default:0)"

	echo "-----------------other-------------------------------------------"
	echo "persist.camera.imglib.hdr.dump no			(in out no default:no)"
	echo "persist.camera.imglib.cac.dump no			(in out no default:no)"
	echo "persist.camera.max_prev.enable 0			(default:0)"
	echo "persist.camera.tintless.dump 0			(0 or 1 default:0)"
	echo "persist.camera.isp.dump 0				(0 or 1 default:0)"
	echo "persist.camera.imglib.dump 0			(0 or 1 default:0)"
	echo "persist.hwc.enable_vds 0				(0 or 1 default:0)"



}
help

