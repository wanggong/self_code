/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <unistd.h>
#include <pthread.h>
#define UNW_LOCAL_ONLY
#include <../../../external/libunwind/include/libunwind.h>

int aa()
{
	unw_cursor_t cursor; unw_context_t uc;
	unw_word_t ip, sp;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);
	while (unw_step(&cursor) > 0) 
	{
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		printf ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);	
	}
	return 0;
}

int a()
{
	aa();return 0;
}
int b()
{
	a();return 0;
}
int c()
{
	b();return 0;
}

extern "C"
{
#if 0
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
	struct mutex_backtrace* get_mutex_locked_mutex_debug();
	void get_libname(pid_t pid,uintptr_t addr ,char *libname,unsigned long* offset);

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
		printf("mutex:%p,locked for %lld seconds\n",mutex,secs);
		for(i = 0 ; i < MUTEX_BACKTACE_COUNT;i++)
		{
			pc = mutex->backtrace[i];
			if(pc == 0)
			{
				break;
			}
			get_libname(pid,pc ,libname,&offset);
			get_function_name(pc,funcname,&offset_infunc);
			printf("#%02d pc %08lx  %s(%s+%ld) \n",i,offset,libname,funcname,offset_infunc);
		}
	}
	void dump_mutex_backtrace()
	{
		int i = 0;
	 	struct mutex_backtrace *mutex_locked = get_mutex_locked_mutex_debug();
		for(i = 0 ; i < 1024;i++)
		{
			if(mutex_locked[i].mutex!= NULL)
			{
				dump_mutex(&mutex_locked[i]);
			}
		}
	}
#else
	void set_mutex_debug_pid(pid_t pid);
	void dump_mutex_backtrace(); 
#endif    
	void thread_mutex_test()
	{
		pthread_mutex_t mutex;
		pthread_mutex_t mutex2;
		printf("%s:%d\n",__func__,__LINE__);
		set_mutex_debug_pid(getpid());
		printf("%s:%d\n",__func__,__LINE__);
		pthread_mutex_init(&mutex,NULL);
		printf("%s:%d\n",__func__,__LINE__);
		pthread_mutex_init(&mutex2,NULL);
		printf("%s:%d\n",__func__,__LINE__);
		pthread_mutex_lock(&mutex); 
		printf("%s:%d\n",__func__,__LINE__);
		pthread_mutex_lock(&mutex2);
		printf("%s:%d\n",__func__,__LINE__);
		dump_mutex_backtrace();
		pthread_mutex_unlock(&mutex2);
		dump_mutex_backtrace();
		pthread_mutex_unlock(&mutex); 
		dump_mutex_backtrace();
		printf("%s:%d\n",__func__,__LINE__);
	}
}
int main()
{
	c();
	printf("%s:%d\n",__func__,__LINE__);
	thread_mutex_test();
	return 0;
}
