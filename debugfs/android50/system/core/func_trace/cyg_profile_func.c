
#include <utils/Log.h>
#include <stdlib.h>
#include <time.h>

/* Constructor and Destructor Prototypes */
void main_constructor( void )
	__attribute__ ((no_instrument_function, constructor));
void main_destructor( void )
	__attribute__ ((no_instrument_function, destructor));
/* Output trace file pointer */
static FILE *fp;
static int inited = 0;

#define READ_SIZE 4096
int __attribute__ ((no_instrument_function)) write_maps()
{
  FILE *maps_fp , *writemapsfd;
  char maps_file[256];
  char filename[256];
  
  size_t size;
  char *buf = malloc(READ_SIZE);
  
  sprintf(filename,"/data/local/log/trace/maps_%d.txt",getpid());
  sprintf(maps_file , "/proc/%d/maps" , getpid());
  maps_fp = fopen(maps_file , "r");
  if (maps_fp == NULL) {
	  ALOGE("open %s failed" , maps_file);
	  return -1;
  }
  writemapsfd = fopen(filename , "w");
  if (writemapsfd == NULL) {
	  ALOGE("open %s failed" , filename);
	  return -1;
  }
  do {
	size = fread(buf , 1 , READ_SIZE , maps_fp);
	fwrite(buf , 1 , size , writemapsfd);
  }while(size == READ_SIZE);
  fclose(writemapsfd);
  return 0;
}

void main_constructor( void )
{
  char filename[256];
  sprintf(filename,"/data/local/log/trace/trace_%d.txt",getpid());
  fp = fopen( filename, "w" );
  if (fp == NULL){
	  ALOGE("open %s failed" , filename);
	  return;
  } 
  if(write_maps() < 0){
	  return ;
  }
  inited = 1;
}
void main_deconstructor( void )
{
  fclose( fp );
}

#define DUMP(func, call) fprintf(fp , "%s: func = %p, called by = %p\n", __FUNCTION__, func, call)

void __attribute__((__no_instrument_function__)) 
    __cyg_profile_func_enter(void *this_func, void *call_site __attribute__((__unused__)) )
{
    if(inited) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        fprintf(fp , "e %8d %p %lu %lu\n", gettid(), this_func , ts.tv_sec,ts.tv_nsec);
    }
}
void __attribute__((__no_instrument_function__))
    __cyg_profile_func_exit(void *this_func, void *call_site __attribute__((__unused__)) )
{
    if(inited){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        fprintf(fp , "x %8d %p %lu %lu\n", gettid(), this_func , ts.tv_sec,ts.tv_nsec);
    }
}



//Android.mk
//LOCAL_CFLAGS  += -finstrument-functions

