
#include <cutils/properties.h>
#include <common_time/local_clock.h>
#include <wgz_debug/wgz_debug.h>
#include <utils/Log.h>


#ifdef __cplusplus
extern "C" 
{
#endif

struct dumped_file
{
	FILE *mAudioDumpFile;
	int64_t lastWriteTime;
	int surfix;
};
/**************************************************
2000:NuPlayer
3000:AudioTrack before
3001:AudioTrack after PCM_8_BIT,copy to audiobuffer
3002:AudioTrack after Other PCM,copy to audiobuffer
4001:one track ,before process__OneTrack16BitsStereoNoResampling
4002:one track ,after process__OneTrack16BitsStereoNoResampling format:AUDIO_FORMAT_PCM_FLOAT
4003:one track ,after process__OneTrack16BitsStereoNoResampling format:AUDIO_FORMAT_PCM_16_BIT need boosted
4004:one track ,after process__OneTrack16BitsStereoNoResampling format:AUDIO_FORMAT_PCM_16_BIT
45X2:AudioMixer Track is X,after RESAMPLE
45X3:AudioMixer Track is X,No RESAMPLE before track.hook
45X4:AudioMixer Track is X,No RESAMPLE after track.hook
48X1:AudioMixer Track is X,after the same mainBuffer mixer, before convertMixerFormat
48X2:AudioMixer Track is X,after the same mainBuffer mixer, after convertMixerFormat
5000:ThreadLoop after mixer ,format:float rate:48000
5001:ThreadLoop after mixer ,format:PCM16
5002:ThreadLoop after effect, before format convert
5003:ThreadLoop after effect, after format convert
9999:ThreadLoop write to hardware
****************************************************/
void write_to_file_for_audio_2s(char *buffer , int numBytes,int surfix,audio_format_t format)
{
	char filename[512]={0};
	int64_t systemtime = systemTime();
	static dumped_file files[64];
	dumped_file *file = NULL;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.audio.dump", value, "0"); 
	if(numBytes <= 0)
	{
		return ;
	}
	if(atoi(value) == 0)
	{
		return;
	}
	for(int i =0;i<64;i++)
	{
		if(files[i].surfix == surfix)
		{
			file = &files[i];
			break;
		}
	}
	if(file != NULL)
	{
		if(systemtime > file->lastWriteTime+2*1000000000LL)
		{
			fclose(file->mAudioDumpFile);
			file->surfix = 0;
			file->lastWriteTime = 0;
			file = NULL;
		}
	}

	if(file == NULL)
	{
		char teeTime[16];
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm tm;
		for(int i =0;i<64;i++)
		{
			if(files[i].surfix == 0)
			{
				file = &files[i];
				break;
			}
		}
		if(file == NULL)
		{
			ALOGE("wgz too much file opened");
			return;
		}
        localtime_r(&tv.tv_sec, &tm);
        strftime(teeTime, sizeof(teeTime), "%Y%m%d%H%M%S", &tm);
        sprintf(filename, "/data/misc/media/%s_%x_%d.raw", teeTime,format,surfix);
		
		file->mAudioDumpFile = fopen(filename, "w+b");
		if(file->mAudioDumpFile == NULL)
		{
			ALOGE("wgz open file %s error(%s)",filename,strerror(errno));
			return;
		}
		file->surfix = surfix;
	}
	ALOGE("wgz start write to file size:%d , surfix=%d",numBytes,surfix);
	fwrite(buffer, 1, numBytes, file->mAudioDumpFile);
	file->lastWriteTime = systemtime;
}



#ifdef __cplusplus
}
#endif

