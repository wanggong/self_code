/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "CallStack"

#include <utils/CallStack.h>
#include <utils/Printer.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <UniquePtr.h>

#include <backtrace/Backtrace.h>
#include <../../../external/libunwind/include/libunwind.h>

namespace android {

CallStack::CallStack() {
}

CallStack::CallStack(const char* logtag, int32_t ignoreDepth) {
    this->update(ignoreDepth+1);
    this->log(logtag);
}

CallStack::~CallStack() {
}

void CallStack::update(int32_t ignoreDepth, pid_t tid) {
    mFrameLines.clear();

    UniquePtr<Backtrace> backtrace(Backtrace::Create(BACKTRACE_CURRENT_PROCESS, tid));
    if (!backtrace->Unwind(ignoreDepth)) {
        ALOGW("%s: Failed to unwind callstack.", __FUNCTION__);
    }
    for (size_t i = 0; i < backtrace->NumFrames(); i++) {
      mFrameLines.push_back(String8(backtrace->FormatFrameData(i).c_str()));
    }
}

void CallStack::log(const char* logtag, android_LogPriority priority, const char* prefix) const {
    LogPrinter printer(logtag, priority, prefix, /*ignoreBlankLines*/false);
    print(printer);
}

void CallStack::dump(int fd, int indent, const char* prefix) const {
    FdPrinter printer(fd, indent, prefix);
    print(printer);
}

String8 CallStack::toString(const char* prefix) const {
    String8 str;

    String8Printer printer(&str, prefix);
    print(printer);

    return str;
}

void CallStack::print(Printer& printer) const {
    for (size_t i = 0; i < mFrameLines.size(); i++) {
        printer.printLine(mFrameLines[i]);
    }
}
#if 1 //wgz add
CallStack::CallStack(const char* logtag, pid_t pid, pid_t tid,int32_t ignoreDepth)
{
	this->update(ignoreDepth+1,pid,tid);
    this->log(logtag);
}

void CallStack::update(int32_t ignoreDepth, pid_t pid, pid_t tid)
{
    mFrameLines.clear();

    UniquePtr<Backtrace> backtrace(Backtrace::Create(pid, tid));
    if (!backtrace->Unwind(ignoreDepth)) {
        ALOGW("%s: Failed to unwind callstack.", __FUNCTION__);
    }
    for (size_t i = 0; i < backtrace->NumFrames(); i++) {
      mFrameLines.push_back(String8(backtrace->FormatFrameData(i).c_str()));
    }
}

void get_libname2(pid_t pid,uintptr_t addr ,char *libname,unsigned long *offset)
{
	static pid_t pid_saved = -1;
	static BacktraceMap *map = NULL;
	if(pid != pid_saved)
	{
		if(map != NULL)
		{
			delete map;
			map = NULL;
		}
	}
	if(map == NULL)
	{
		map = BacktraceMap::Create(pid);
	}
	if(map != NULL)
	{
		const backtrace_map_t *bm = map->Find(addr);
		if(bm != NULL)
		{
			strcpy(libname,bm->name.c_str());
			*offset = addr -bm->start;
		}
	}
}
#endif

}; // namespace android
#if 1 //wgz add for backtrace
//pid = -1 , if dump the current process , else use getpid() get the pid;
//tid = -1 , if dump the current thread,else use gettid() get the tid;
extern "C" 
{
	void dobacktrace(pid_t pid,pid_t tid)
	{
		android::CallStack cs("wgz",pid,tid);
	}

	void get_libname(pid_t pid,uintptr_t addr ,char *libname ,unsigned long *offset)
	{
		android::get_libname2(pid,addr ,libname,offset);
	}

	#define MUTEX_BACKTACE_COUNT 32
	void get_function_name(unsigned long pc, char *buf ,unsigned long *offset)
	{
		*offset = 0;
		unw_word_t value;
		unw_context_t uc;
		unw_getcontext(&uc);
		if (unw_get_proc_name_by_ip(unw_local_addr_space, pc, buf, 512,
		                      	&value, &uc) >= 0 && buf[0] != '\0') 
		{
			*offset = static_cast<uintptr_t>(value);
		}
	}

	struct mutex_backtrace
	{
		long unsigned backtrace[MUTEX_BACKTACE_COUNT];
		unsigned long long lock_time_sec;
		pthread_mutex_t* mutex;
	};
	void set_mutex_debug_pid(pid_t pid);
	void set_mutex_dump_tid(pid_t tid);
	struct mutex_backtrace* get_mutex_locked_mutex_debug();
	void get_libname(pid_t pid,uintptr_t addr ,char *libname,unsigned long* offset);
	pid_t gettid();

	void dump_mutex(struct mutex_backtrace *mutex)
	{
		int i = 0;
		unsigned long pc = 0;
		pid_t pid = getpid();
		char libname[512] = {0}; 
		char funcname[512] = {0};
		unsigned long offset = 0;
		unsigned long offset_infunc = 0;
		unsigned long long secs = time(NULL) - mutex->lock_time_sec;
		ALOGE("mutex:%p,locked for %lld seconds\n",mutex,secs);
		for(i = 0 ; i < MUTEX_BACKTACE_COUNT;i++)
		{
			pc = mutex->backtrace[i];
			if(pc == 0)
			{
				break;
			}
			get_libname(pid,pc ,libname,&offset);
			get_function_name(pc,funcname,&offset_infunc);
			ALOGE("#%02d pc %08lx  %s(%s+%ld) \n",i,offset,libname,funcname,offset_infunc);
		}
	}
	void dump_mutex_backtrace()
	{
		int i = 0;
		set_mutex_debug_pid(gettid());
	 	struct mutex_backtrace *mutex_locked = get_mutex_locked_mutex_debug();
		for(i = 0 ; i < 1024;i++)
		{
			if(mutex_locked[i].mutex!= NULL)
			{
				dump_mutex(&mutex_locked[i]);
			}
		}
	}
}

#endif //wgz add end
