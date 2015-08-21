#ifndef WGZ_DEBUG
#define WGZ_DEBUG

#include <stdio.h>
#include <system/audio.h>

#ifdef __cplusplus
extern "C" 
{
#endif
void write_to_file_for_audio_2s(char *buffer , int numBytes,int surfix,audio_format_t format);


#ifdef __cplusplus
}
#endif
#endif
//LOCAL_SHARED_LIBRARIES := libcorkscrew
//LOCAL_SHARED_LIBRARIES := libandroid_backtrace