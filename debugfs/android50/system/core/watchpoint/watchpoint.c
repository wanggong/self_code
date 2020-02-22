

#include <sys/ptrace.h>
#include <elf.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <unistd.h>
#include <linux/ptrace.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/hw_breakpoint.h>

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>


#define BACKTRACE_COUNT 1000
static inline void print_backtrace(void)
{
  void* array[BACKTRACE_COUNT] = {0};
  int size = 0;
  char **strframe = 0;
  int i = 0, j = 0;
  
  size = backtrace(array, BACKTRACE_COUNT);
  strframe = backtrace_symbols(array, size);
  
  printf("callstack:\n");
  for(i = 0; i < size; i++){
    printf("frame %d -- %s\n", i, strframe[i]);
  }
  
  if(strframe)
  {
    free(strframe);
    strframe = 0;
  }
}




// Host libc does not define this.
#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 4
#endif

//#define printf(fmt,args...) ALOGE("wgz:" fmt,##args)


#define BP_CREATE 1
#define BP_DEL 2

struct WatchPoint
{
	int m_fd;
    struct sigaction action;
    void* addr;
};

static struct WatchPoint self ;

static void signal_handler(int signal_number, siginfo_t* info, void*ignore) {
    signal_number = signal_number;
    info = info;
    printf("signal_number : %d, tid:%d ,sno:%d , pc:%p \n" , signal_number, ( int)syscall(__NR_gettid) , info->si_signo,info->si_addr);   
    print_backtrace();
}


static int watchpoint_init()
{
    self.m_fd = open("/dev/breakpoint" , O_RDWR);
    if(self.m_fd < 0){
        printf("open file failed\n");
		return -1;
    }
    memset(&self.action, 0, sizeof(self.action));
    sigemptyset(&self.action.sa_mask);
    self.action.sa_sigaction = signal_handler;
    self.action.sa_flags = SA_RESTART | SA_SIGINFO;
    if(sigaction(SIGUSR1, &self.action, 0)<0){
		printf("install signal failed\n");
        close(self.m_fd);
        self.m_fd = 0;
        return -1;
    }
    return 0;
}


int add_watchpoint(void *addr){
	struct perf_event_attr user_attr;
	
	user_attr.bp_addr = (__u64)addr;
	user_attr.bp_len = 4;
	user_attr.bp_type = HW_BREAKPOINT_W;
	if(ioctl(self.m_fd, BP_CREATE , &user_attr) < 0){
		printf("add_watchpoint error %s\n",strerror(errno));
		return -1;
	}
	self.addr = addr;
	printf("add watchpoint success %p\n" , self.addr);
	return 0;
}
int del_watchpoint(){
	struct perf_event_attr user_attr;
	
	user_attr.bp_addr = (__u64)self.addr;
	user_attr.bp_len = 4;
	user_attr.bp_type = HW_BREAKPOINT_W;
	if(ioctl(self.m_fd , BP_DEL , &user_attr) < 0){
		printf("del_watchpoint error %s\n",strerror(errno));
		return -1;
	}
	
	printf("del_watchpoint sucess %p\n" , self.addr);
	return 0;
}

int start_monitor(void *addr)
{
    int rc = watchpoint_init();
   if(rc < 0){
   	printf("init watchpoint failed\n");
	return -1;
   }
   rc = add_watchpoint(addr);
   if(rc < 0){
   	printf("add watchpoint failed\n");
	return -1;
   }
   return 0;
}

int stop_monitor()
{
    int rc = del_watchpoint();
    if(rc < 0) {
       printf("del watchpoint failed\n"); 
    }
    close(self.m_fd);
    return 0;
}



volatile int watchvar = 0;


void b()
{
     print_backtrace();

    printf("child pid:%d,tid:%d\n",getpid(),( int)syscall(__NR_gettid));
    printf("child start trigger\n");
    watchvar = 3;
    printf("child end trigger\n");
}

void* threadnew2(void *ignore)
{
    b();
    return 0;
}

int main() {
    int *p = 0;
    int a = 0;

    int rc = 0;

    start_monitor((void*)&watchvar);

    pthread_t thread;


    pthread_create(&thread , 0 , threadnew2,0);

    pthread_join(thread , 0);

    printf("main pid:%d,tid:%d\n",getpid(),( int)syscall(__NR_gettid));
    printf("%d\n", watchvar);

    printf("main write start trigger\n");
    watchvar = 5;
    printf("main write end trigger\n");

    printf("main read start trigger\n");
    a = watchvar;
    printf("main read end trigger\n");


    stop_monitor();
    printf("after stop start trigger\n");
    watchvar = 10;
    printf("after stop end trigger\n");

    return 0;
}



