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

// Host libc does not define this.
#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 4
#endif

#define printf(fmt,args...) ALOGE("wgz:" fmt,##args)

void* threadnew(void *)
{
    int i = 0;
    printf("new thread pid:%d,tid:%d",getpid(),gettid());
    while(1){
    printf("wgz i:%d" , i++);
    usleep(1000*1000*1);
    }
    
}
static void  fork_child(int *data) {
  // Extra precaution: make sure we go away if anything happens to our parent.
//  if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) == -1) {
//    printf("prctl(PR_SET_PDEATHSIG)");
//    _exit(1);
//  }
    pthread_t thread;
  printf("child pid:%d,tid:%d",getpid(),gettid());

  pthread_create(&thread , 0 , threadnew,0);
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
    printf("ptrace(PTRACE_TRACEME)");
    _exit(3);
  }
  raise(SIGSTOP); // Synchronize with the tracer, let it set the watchpoint.
  *data = 1; // Now trigger the watchpoint.
  _exit(0);
}
static int are_watchpoints_supported(pid_t child) {
    
  struct user_hwdebug_state dreg_state;
  struct iovec iov;
  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof(dreg_state);
  long result = ptrace(PTRACE_GETREGSET, child, NT_ARM_HW_WATCH, &iov);
  if (result == -1) {
    printf("not support watchpoint");
    return 0;
  }
  printf("get hw watch %d,%d" , (dreg_state.dbg_info>>8)&0xff , dreg_state.dbg_info & 0xff);
  return (dreg_state.dbg_info & 0xff) > 0;
}

static void set_watchpoint(pid_t child, const void *address, size_t size) {
    
  const unsigned byte_mask = (1 << size) - 1;
  const unsigned type = HW_BREAKPOINT_W; // Write.
  const unsigned enable = 1;
  const unsigned control = byte_mask << 5 | type << 3 | enable;
  struct user_hwdebug_state dreg_state;
  memset(&dreg_state, 0, sizeof dreg_state);
  dreg_state.dbg_regs[0].addr = (__u64)address;
  dreg_state.dbg_regs[0].ctrl = control;
  struct iovec iov;
  iov.iov_base = &dreg_state;
  iov.iov_len = offsetof(struct user_hwdebug_state, dbg_regs) + sizeof(dreg_state.dbg_regs[0]);
  ptrace(PTRACE_SETREGSET, child, NT_ARM_HW_WATCH, &iov);
}

//static void dump_backtrace(pid_t pid , pid_t tid)
//{
    //android::CallStack cs("wgz",pid,tid);
//}

static void run_watchpoint_test_impl() {
  int data = 0;
  printf("parent pid:%d,tid:%d",getpid(),gettid());
  
  pid_t child = fork();
  if (child == 0) fork_child(&data);
  printf("child:%d",child);
  
  usleep(1000*1000*20);
  
  int status;
  if(child != waitpid(child, &status, __WALL)){
    printf("waitpid error\n");
  }
  if(!WIFSTOPPED(status)) {
    printf("error");
   }
  if(SIGSTOP != WSTOPSIG(status)) {
    printf("stop sig not SIGSTOP");
  }
  if (!are_watchpoints_supported(child)) {
    printf("not support watchpoints");
    return;
  }
  set_watchpoint(child, &data, sizeof(data));
  
  if(ptrace(PTRACE_CONT, child, 0, 0) < 0) {
    printf("continue error");
  }
  if(child != waitpid(child, &status, __WALL)) {
    printf("waitpid error\n");
  }
  if(!WIFSTOPPED(status)) {
    printf("error");
  }
  if(SIGTRAP!= WSTOPSIG(status)){
    printf("error , signal is not SIGTRAP");
  }
  siginfo_t siginfo;
  if(0 != ptrace(PTRACE_GETSIGINFO, child, 0, &siginfo)){
    printf("error , PTRACE_GETSIGINFO error");
  }
  if(TRAP_HWBKPT != siginfo.si_code){
    printf("error , si_code is not TRAP_HWBKPT");
  }
  if(&data != siginfo.si_addr){
    printf("error , data != siginfo.si_addr");
  }
  
  printf("watchpoint run ok tid:%d" , siginfo.si_pkey);
}

int main() {
    run_watchpoint_test_impl();
    return 0;
}
