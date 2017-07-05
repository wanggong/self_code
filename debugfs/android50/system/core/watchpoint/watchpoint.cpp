/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sys/ptrace.h>
#include <elf.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <unistd.h>
#include <utils/Log.h>
#include <linux/ptrace.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/hw_breakpoint.h>
#include <backtrace/Backtrace.h>
#include <utils/CallStack.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <vector>


// Host libc does not define this.
#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 4
#endif

#define printf(fmt,args...) ALOGE("wgz:" fmt,##args)


#define BP_CREATE 1
#define BP_DEL 2
class WatchPoint
{
public:
    WatchPoint(){
        m_fd = open("/dev/breakpoint" , O_RDWR);
        if(m_fd < 0){
            printf("open file failed");
        }
        memset(&action, 0, sizeof(action));
        sigemptyset(&action.sa_mask);
        action.sa_sigaction = signal_handler;
        action.sa_flags = SA_RESTART | SA_SIGINFO;
        if(sigaction(SIGUSR1, &action, nullptr)<0){
            close(m_fd);
            m_fd = 0;
        }
    }
    ~WatchPoint(){
        if(m_watchaddr_vec.size() != 0){
            del_watchpoint(m_watchaddr_vec.front());
        }
        if(m_fd > 0){
            close(m_fd);
        }
    }
    int add_watchpoint(void *addr , int length = 4 ,int type = HW_BREAKPOINT_W){
        perf_event_attr user_attr;
        std::vector<void*>::iterator result;
        
        result= find( m_watchaddr_vec.begin( ), m_watchaddr_vec.end( ), addr );
        if ( result != m_watchaddr_vec.end() ) {
            printf("error , %p has added before" , addr);
            return -1;
        }
        
        user_attr.bp_addr = (__u64)addr;
        user_attr.bp_len = length;
        user_attr.bp_type = type;
        if(ioctl(m_fd, BP_CREATE , &user_attr) < 0){
            printf("add_watchpoint error %s",strerror(errno));
            return -1;
        }
        m_watchaddr_vec.push_back(addr);
        printf("add watchpoint sucess %p" , addr);
        return 0;
    }
    int del_watchpoint(void *addr){
        perf_event_attr user_attr;
        std::vector<void*>::iterator result;
        
        result= find( m_watchaddr_vec.begin( ), m_watchaddr_vec.end( ), addr );
        if ( result == m_watchaddr_vec.end() ) {
            printf("error , can not find %p" , addr);
            return -1;
        }
        
        user_attr.bp_addr = (__u64)addr;
        user_attr.bp_len = 4;
        user_attr.bp_type = HW_BREAKPOINT_W;
        if(ioctl(m_fd , BP_DEL , &user_attr) < 0){
            printf("del_watchpoint error %s",strerror(errno));
            return -1;
        }
        
        m_watchaddr_vec.erase(result);
        printf("del_watchpoint sucess %p" , addr);
        return 0;
    }

private:
    static void signal_handler(int signal_number, siginfo_t* info, void*) {
        signal_number = signal_number;
        info = info;
        printf("tid:%d ,sno:%d , pc:%p" , gettid() , info->si_signo,info->si_addr);
        android::CallStack s("wgz");    
    }

private:
    int m_fd;
    struct sigaction action;
    std::vector<void*> m_watchaddr_vec;
};

int watchvar = 0;


void b()
{
    printf("child pid:%d,tid:%d",getpid(),gettid());
    printf("wgz start trigger");
    watchvar = 3;
    printf("wgz end trigger");
}

void* threadnew2(void *)
{
    b();
    return 0;
}

int main() {
    int *p = 0;
    
   WatchPoint *w = new WatchPoint();
   w->add_watchpoint(&watchvar);
  pthread_t thread;
  printf("main pid:%d,tid:%d",getpid(),gettid());

  pthread_create(&thread , 0 , threadnew2,0);
  
  pthread_join(thread , 0);

 printf("again start trigger");
watchvar = 5;
printf("again end trigger");

w->del_watchpoint(&watchvar);
printf("after del start trigger");
watchvar = 5;
printf("after del end trigger");

//    run_watchpoint_test_impl();
    return 0;
}
