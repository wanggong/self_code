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

#include <link.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <log/log.h>

#include <map>
#include <backtrace/BacktraceMap.h>
#define UNW_LOCAL_ONLY
#include <../../../external/libunwind/include/libunwind.h>

#define ERR(fmt, args...) ALOGE("MUTEX "fmt, ##args)
#define DEBUG(fmt, args...) do{}while(0)
//#define DEBUG(fmt, args...) ALOGE("MUTEX "fmt, ##args)

#if 0
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
		DEBUG ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);	
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
		DEBUG("mutex:%p,locked for %lld seconds\n",mutex,secs);
		for(i = 0 ; i < MUTEX_BACKTACE_COUNT;i++)
		{
			pc = mutex->backtrace[i];
			if(pc == 0)
			{
				break;
			}
			get_libname(pid,pc ,libname,&offset);
			get_function_name(pc,funcname,&offset_infunc);
			DEBUG("#%02d pc %08lx  %s(%s+%ld) \n",i,offset,libname,funcname,offset_infunc);
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
		DEBUG("%s:%d\n",__func__,__LINE__);
		set_mutex_debug_pid(getpid());
		DEBUG("%s:%d\n",__func__,__LINE__);
		pthread_mutex_init(&mutex,NULL);
		DEBUG("%s:%d\n",__func__,__LINE__);
		pthread_mutex_init(&mutex2,NULL);
		DEBUG("%s:%d\n",__func__,__LINE__);
		pthread_mutex_lock(&mutex); 
		DEBUG("%s:%d\n",__func__,__LINE__);
		pthread_mutex_lock(&mutex2);
		DEBUG("%s:%d\n",__func__,__LINE__);
		dump_mutex_backtrace();
		pthread_mutex_unlock(&mutex2);
		dump_mutex_backtrace();
		pthread_mutex_unlock(&mutex); 
		dump_mutex_backtrace();
		DEBUG("%s:%d\n",__func__,__LINE__);
	}
}
#endif

#if 0 //wgz add
extern "C"
{
extern void (*mutex_lock_debug)(pthread_mutex_t* mutex, int shared);
extern void (*mutex_unlock_debug)(pthread_mutex_t* mutex, int shared);
extern void (*mutex_destroy_debug)(pthread_mutex_t* mutex);
extern void (*dlmalloc_exit)(void *mem,size_t bytes);
extern void (*dlmalloc_enter)(size_t bytes);
extern int is_pthread_key_lock(pthread_mutex_t* mutex);
};

static pthread_key_t mutex_key = -1;

#define MUTEX_BACKTACE_COUNT 32
struct mutex_backtrace
{
	pthread_mutex_t* mutex ;
	unw_word_t backtrace[MUTEX_BACKTACE_COUNT];
};
struct mutex_debug
{
	std::map<pthread_mutex_t*,mutex_backtrace*> mutex_map;
	int entered_count;
	pid_t thread_id;
};
static std::map<pthread_mutex_t*,mutex_backtrace*> backtrace_map;
static pthread_mutex_t backtrace_map_mutex = PTHREAD_MUTEX_INITIALIZER;


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

void get_libname(pid_t pid,uintptr_t addr ,char *libname,unsigned long *offset)
{
	static pid_t pid_saved = -1;
	static BacktraceMap *map = NULL;
	//DEBUG("%s:%d,pid:%d\n",__func__,__LINE__,pid);
#if 0
	if(pid != pid_saved)
	{
		if(map != NULL)
		{
			delete map;
			map = NULL;
		}
	}
#endif
	//DEBUG("%s:%d\n",__func__,__LINE__);
	if(map == NULL)
	{
		map = BacktraceMap::Create(pid);
	}

	if(map != NULL)
	{
		//DEBUG("%s:%d , addr:0x%x\n",__func__,__LINE__,addr);
		backtrace_map_t *bm = (backtrace_map_t *)map->Find(addr);
		if(bm != NULL)
		{
			//DEBUG("%s:%d\n",__func__,__LINE__);
			*offset = addr -bm->start;
		}
		Dl_info info;
	    if (dladdr((void*) addr, &info) != 0) 
		{
	      strcpy(libname, info.dli_fname);
	    }
	}
	//DEBUG("%s:%d\n",__func__,__LINE__);
}


void dump_backtrace(mutex_backtrace *backtrace)
{
	DEBUG("mutex:%p\n",backtrace->mutex);
	unw_word_t pc = 0;
	pid_t pid = getpid();
	char libname[512] = {0}; 
	char funcname[512] = {0};
	unsigned long offset = 0;
	unsigned long offset_infunc = 0;
	for(int i = 0; i < MUTEX_BACKTACE_COUNT;++i)
	{
		pc = backtrace->backtrace[i];
		if(pc == 0)
		{
			break;
		}
		//DEBUG("%s:%d\n",__func__,__LINE__);
		get_libname(pid,pc ,libname,&offset);
		//DEBUG("%s:%d\n",__func__,__LINE__);
		get_function_name(pc,funcname,&offset_infunc);
		ERR("#%02d pc %08lx  %s(%s+%ld) \n",i,offset,libname,funcname,offset_infunc);
	}
}

extern "C" void dump_all_mutex()
{
	std::map<pthread_mutex_t*,mutex_backtrace*>::iterator iter;
	for(iter = backtrace_map.begin();iter != backtrace_map.end();++iter)
	{
		dump_backtrace(iter->second);
	}
}

int unwind_backtrace(unw_word_t backtrace[],int count)
{
	unw_word_t ip, sp;
	int i = 0;
	unw_cursor_t *cursor = (unw_cursor_t*)malloc(sizeof(unw_cursor_t));
	if(cursor == NULL)
	{
		return -1;
	}
	unw_context_t *uc = (unw_context_t*)malloc(sizeof(unw_context_t));
	if(uc == NULL)
	{
		free(cursor);
		return -1;
	}
	memset(cursor,0,sizeof(unw_cursor_t));
	memset(uc,0,sizeof(unw_context_t));

	unw_getcontext(uc);
	unw_init_local(cursor, uc);
	for( i = 0 ; i < count ; i++)
	{
		if(unw_step(cursor) > 0) 
		{
			unw_get_reg(cursor, UNW_REG_IP, &ip);
			//unw_get_reg(cursor, UNW_REG_SP, &sp);
			backtrace[i] = ip;
			//DEBUG("ip = %lx\n", (long) ip);	
		}
		else
		{
			break; 
		}
	}
	if(i<count)
	{
		backtrace[i] = 0;
	}
	
	free(cursor);
	free(uc);
	return 0;
}

static pthread_mutex_t backtrace_mutex = PTHREAD_MUTEX_INITIALIZER;
mutex_backtrace* do_backtrace(pthread_mutex_t* mutex)
{
	DEBUG("%s:%d\n",__func__,__LINE__);
	mutex_backtrace *mb = new mutex_backtrace();
	mb->mutex = mutex; 
	pthread_mutex_lock(&backtrace_mutex);
	unwind_backtrace(mb->backtrace,MUTEX_BACKTACE_COUNT);
	pthread_mutex_unlock(&backtrace_mutex);
	DEBUG("%s:%d\n",__func__,__LINE__);
	return mb;
}
int is_ignored_lock(pthread_mutex_t* mutex)
{
	return is_pthread_key_lock(mutex)||mutex==&backtrace_map_mutex;
}
mutex_debug* create_mutex_if_needed(pthread_mutex_t* mutex)
{
	if(is_ignored_lock(mutex))
	{
		return 0;
	}
	mutex_debug *debug = (mutex_debug *)pthread_getspecific(mutex_key);
	if(debug == 0)
	{
		DEBUG("%s:%d create debug\n",__func__,__LINE__);
		pthread_setspecific(mutex_key,(void*)0x12345678);
		debug = new mutex_debug();
		debug->entered_count = 0;
		debug->thread_id = getpid();
		pthread_setspecific(mutex_key,debug);
		DEBUG("%s:%d create debug end pid:%d\n",__func__,__LINE__,debug->thread_id);
	}
	else if(debug == (void*)0x12345678)//now alloc mutex_debug
	{
		return 0;
	}
	else
	{
		DEBUG("%s:%d got entered_count:%d,threadid:%d\n"
			,__func__,__LINE__,debug->entered_count,debug->thread_id);
	}
	return debug;
}
mutex_debug* inc_mutex_status_if_needed(pthread_mutex_t* mutex)
{
	mutex_debug *debug = create_mutex_if_needed(mutex);
	if(debug != 0)
	{	
		DEBUG("%s:%d entered_count:%d\n",__func__,__LINE__,debug->entered_count);
		debug->entered_count++;
		return debug;
	}
	else
	{
		return 0;
	}
}

mutex_debug* get_mutex_debug(pthread_mutex_t* mutex)
{
	if(is_ignored_lock(mutex))
	{
		return 0;
	}
	mutex_debug *debug = (mutex_debug *)pthread_getspecific(mutex_key);
	if(debug == 0||debug==(void*)0x12345678)
	{
		return 0;
	}
	return debug;
}

mutex_debug* dec_mutex_status_if_needed(pthread_mutex_t* mutex)
{
	mutex_debug *debug = get_mutex_debug(mutex);
	if(debug != 0)
	{
		debug->entered_count--;
	}
	return debug;
}
void dlmalloc_enter_impl(size_t bytes __unused)
{
	DEBUG("%s:%d\n",__func__,__LINE__);
	inc_mutex_status_if_needed(0);
}
void dlmalloc_exit_impl(void *mem __unused,size_t bytes __unused)
{
	DEBUG("%s:%d\n",__func__,__LINE__);
	dec_mutex_status_if_needed(0);
}

void mutex_lock_debug_impl(pthread_mutex_t* mutex , int shared __unused)
{
	DEBUG("%s:%d:%p\n",__func__,__LINE__,mutex);
	mutex_debug *debug = inc_mutex_status_if_needed(mutex);
	if(debug == 0)
	{
		return;
	}
	else if(debug->entered_count != 1)
	{
		dec_mutex_status_if_needed(mutex);
		return;
	}
	DEBUG("%s:%d:%p entered_count is 1 do backtrace\n",__func__,__LINE__,mutex);
	mutex_backtrace *mb = do_backtrace(mutex);
	//debug->mutex_map[mutex] = mb;
	pthread_mutex_lock(&backtrace_map_mutex);
	backtrace_map[mutex] = mb;
	pthread_mutex_unlock(&backtrace_map_mutex);
	dec_mutex_status_if_needed(mutex);
}


void mutex_unlock_debug_impl(pthread_mutex_t* mutex , int shared __unused)
{
	DEBUG("%s:%d:%p\n",__func__,__LINE__,mutex);
	mutex_debug *debug = inc_mutex_status_if_needed(mutex);
	if(debug == 0)
	{
		return;
	}
	std::map<pthread_mutex_t*,mutex_backtrace*>::iterator iter;
	iter = backtrace_map.find(mutex);
	if(iter == backtrace_map.end())
	{
		DEBUG("%s:%d:%p no mutex,return\n",__func__,__LINE__,mutex);
		dec_mutex_status_if_needed(mutex);
		return;
	}
	mutex_backtrace *mb = iter->second;
	delete mb;
	pthread_mutex_lock(&backtrace_map_mutex);
	backtrace_map.erase(iter);
	pthread_mutex_unlock(&backtrace_map_mutex);
	dec_mutex_status_if_needed(mutex);
}
void mutex_destroy_debug_impl(pthread_mutex_t* mutex __unused) 
{
	DEBUG("%s:%d:%p\n",__func__,__LINE__,mutex);
}

extern "C" void init_mutex_debug()
{
	if(pthread_key_create(&mutex_key,NULL))
	{
		DEBUG("error pthread_key_create failed\n");
		return;
	}
	mutex_lock_debug = mutex_lock_debug_impl;
	mutex_unlock_debug = mutex_unlock_debug_impl;
	mutex_destroy_debug = mutex_destroy_debug_impl;
	dlmalloc_enter = dlmalloc_enter_impl;
	dlmalloc_exit = dlmalloc_exit_impl;
}

void uninit_mutex_debug()
{
	mutex_lock_debug = 0;
	mutex_unlock_debug = 0;
	mutex_destroy_debug = 0;
	dlmalloc_enter = 0;
	dlmalloc_exit = 0;
	pthread_key_delete(mutex_key);
}

#endif


#if 1 //wgz add
extern "C"
{
extern void (*mutex_lock_debug)(pthread_mutex_t* mutex, int shared);
extern void (*mutex_unlock_debug)(pthread_mutex_t* mutex, int shared);
extern void (*mutex_destroy_debug)(pthread_mutex_t* mutex);
int is_thread_mutex(pthread_mutex_t *mutex);
int is_gm_mutex(void *mutex);

};


#define MUTEX_BACKTACE_COUNT 32
struct mutex_backtrace
{
	pthread_mutex_t* mutex ;
	pid_t pid;
	unw_word_t backtrace[MUTEX_BACKTACE_COUNT];
	mutex_backtrace *next;
};
struct mutex_backtrace_list
{
	mutex_backtrace *used;
	mutex_backtrace *freed;
	pthread_mutex_t lock;
	mutex_backtrace* getone()
	{
		pthread_mutex_lock(&lock);
		mutex_backtrace *temp = freed;
		if(temp != NULL)
		{
			freed = temp->next;
			temp->next = used;
			used = temp;
		}
		pthread_mutex_unlock(&lock);
		return temp;
	}

	void putone(pthread_mutex_t* mutex)
	{
		pthread_mutex_lock(&lock);
		mutex_backtrace *temp = used;
		mutex_backtrace *pre = NULL;
		pid_t tid = gettid();
		while(temp != NULL)
		{
			if(temp->mutex == mutex)
			{
				if(tid!=temp->pid)
				{
					ERR("error , curtid[%d],lockedtid[%d],mutex[%p]",tid,temp->pid,mutex);
					return;
				}
				if(pre != NULL)
				{
					pre->next = temp->next;
				}
				else
				{
					used = temp->next;
				}
				break;
			}
			pre = temp;
			temp=temp->next;
		}
		if(temp != NULL)
		{
			temp->next = freed;
			freed = temp;
			temp->backtrace[0] = 0;
			temp->mutex = 0;
			temp->pid = 0;
		}
		pthread_mutex_unlock(&lock);
	}	
	void init(int count = 1024)
	{
		pthread_mutex_init(&lock,NULL);
		freed = used = NULL;
		for(int i = 0 ; i < count ; ++i)
		{
			mutex_backtrace *temp = new mutex_backtrace();
			temp->next = freed;
			freed = temp;
		}
	}
	void uninit()
	{
		pthread_mutex_destroy(&lock);
	}
};

mutex_backtrace_list bt_list;

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

void get_libname(pid_t pid,uintptr_t addr ,char *libname,unsigned long *offset)
{
	static pid_t pid_saved = -1;
	static BacktraceMap *map = NULL;
	//DEBUG("%s:%d,pid:%d\n",__func__,__LINE__,pid);
#if 0
	if(pid != pid_saved)
	{
		if(map != NULL)
		{
			delete map;
			map = NULL;
		}
	}
#endif
	//DEBUG("%s:%d\n",__func__,__LINE__);
	if(map == NULL)
	{
		map = BacktraceMap::Create(pid);
	}

	if(map != NULL)
	{
		//DEBUG("%s:%d , addr:0x%x\n",__func__,__LINE__,addr);
		backtrace_map_t *bm = (backtrace_map_t *)map->Find(addr);
		if(bm != NULL)
		{
			//DEBUG("%s:%d\n",__func__,__LINE__);
			*offset = addr -bm->start;
		}
		Dl_info info;
	    if (dladdr((void*) addr, &info) != 0) 
		{
		  libname[0]='/';
		  libname++;
	      strcpy(libname, info.dli_fname);
	    }
	}
	//DEBUG("%s:%d\n",__func__,__LINE__);
}


void dump_backtrace(mutex_backtrace *backtrace,int index)
{
	ERR("[%04d][%p][%d]>>>>>>>>>>>>>>>>>>>\n",index,backtrace->mutex,backtrace->pid);
	unw_word_t pc = 0;
	pid_t pid = getpid();
	char libname[512] = {0}; 
	char funcname[512] = {0};
	unsigned long offset = 0;
	unsigned long offset_infunc = 0;
	for(int i = 0; i < MUTEX_BACKTACE_COUNT;++i)
	{
		pc = backtrace->backtrace[i];
		if(pc == 0)
		{
			break;
		}
		//DEBUG("%s:%d\n",__func__,__LINE__);
		get_libname(pid,pc ,libname,&offset);
		//DEBUG("%s:%d\n",__func__,__LINE__);
		get_function_name(pc,funcname,&offset_infunc);
		ERR("    #%02d pc %08lx  %s(%s+%ld) \n",i,offset,libname,funcname,offset_infunc);
	}
	ERR("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

int unwind_backtrace(unw_word_t backtrace[],int count)
{
	unw_word_t ip, sp;
	int i = 0;
	unw_cursor_t cursor ;
	unw_context_t uc ;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);
	for( i = 0 ; i < count ; i++)
	{
		if(unw_step(&cursor) > 0) 
		{
			unw_get_reg(&cursor, UNW_REG_IP, &ip);
			//unw_get_reg(cursor, UNW_REG_SP, &sp);
			backtrace[i] = ip;
			//DEBUG("ip = %lx\n", (long) ip);	
		}
		else
		{
			break; 
		}
	}
	if(i<count)
	{
		backtrace[i] = 0;
	}
	return 0;
}

static pthread_mutex_t backtrace_mutex = PTHREAD_MUTEX_INITIALIZER;
static pid_t backtracing_tid[1024]={0};
int is_backtracing()
{
	pid_t tid = gettid();
	for(int i = 0 ; i < 1024;i++)
	{
		if(tid == backtracing_tid[i])
		{
			return 1;
		}
	}
	return 0;
}
void backtrace_start()
{
	pthread_mutex_lock(&backtrace_mutex);
	pid_t tid = gettid();
	for(int i = 0 ; i < 1024;i++)
	{
		if(0 == backtracing_tid[i])
		{
			backtracing_tid[i] = tid;
			break;
		}
	}
	pthread_mutex_unlock(&backtrace_mutex);
	DEBUG("%s:%d start tid:%d\n",__func__,__LINE__,gettid());
}
void backtrace_end()
{
	pthread_mutex_lock(&backtrace_mutex);
	pid_t tid = gettid();
	for(int i = 0 ; i < 1024;i++)
	{
		if(tid == backtracing_tid[i])
		{
			backtracing_tid[i] = 0;
			break;
		}
	}
	pthread_mutex_unlock(&backtrace_mutex);
	DEBUG("%s:%d end tid:%d\n",__func__,__LINE__,gettid());
}

void do_backtrace(mutex_backtrace *mb)
{
	//backtrace_start();
	//pthread_mutex_lock(&backtrace_mutex);
	unwind_backtrace(mb->backtrace,MUTEX_BACKTACE_COUNT);
	//pthread_mutex_unlock(&backtrace_mutex);
	//backtrace_end();
}
int is_ignored_lock(pthread_mutex_t* mutex)
{
	return mutex==&bt_list.lock||mutex==&backtrace_mutex
		||is_thread_mutex(mutex)||is_gm_mutex(mutex);
}

pid_t test_pid =0;
void mutex_lock_debug_impl(pthread_mutex_t* mutex , int shared __unused)
{
	pid_t tid= gettid();
	if(tid == test_pid)
	DEBUG("%s:%d:%p:%d\n",__func__,__LINE__,mutex,gettid());
	if(is_ignored_lock(mutex))
	{
		return;
	}
	if(is_backtracing())
	{
		return;
	}
	backtrace_start();
	mutex_backtrace *mb = bt_list.getone();
	if(mb != NULL)
	{
		mb->mutex = mutex;
		mb->pid = gettid();
		do_backtrace(mb);
	}
	backtrace_end();
}


void mutex_unlock_debug_impl(pthread_mutex_t* mutex , int shared __unused)
{
	//ERR("%s:%d:%p,tid:%d\n",__func__,__LINE__,mutex,gettid());
	if(is_ignored_lock(mutex))
	{
		return;
	}
	if(is_backtracing())
	{
		return;
	}
	//ERR("%s:%d:%p,tid:%d\n",__func__,__LINE__,mutex,gettid());
	backtrace_start();
	bt_list.putone(mutex);
	backtrace_end();
}
void mutex_destroy_debug_impl(pthread_mutex_t* mutex __unused) 
{
#if 0
	pid_t tid = gettid();
	//if(tid == test_pid)
	//ERR("%s:%d:%p,tid:%d\n",__func__,__LINE__,mutex,gettid());
	if(is_ignored_lock(mutex))
	{
		return;
	}
	if(is_backtracing())
	{
		return;
	}
	//if(tid == test_pid)
	//ERR("%s:%d:%p,tid:%d\n",__func__,__LINE__,mutex,gettid());
	backtrace_start();
	bt_list.putone(mutex);
	backtrace_end();
#endif
}

extern "C" void dump_all_mutex()
{
	static int times = 0;
	int index = 0;
	backtrace_start();
	ERR("dump mutex enter[%d]",times);
	pthread_mutex_lock(&bt_list.lock);
	for(mutex_backtrace *temp=bt_list.used;temp != NULL;temp=temp->next)
	{
		dump_backtrace(temp,index++);
	}
	pthread_mutex_unlock(&bt_list.lock);
	ERR("dump mutex exit[%d]",times++);
	backtrace_end();
}


extern "C" void init_mutex_debug()
{
	bt_list.init();
	mutex_lock_debug = mutex_lock_debug_impl;
	mutex_unlock_debug = mutex_unlock_debug_impl;
	mutex_destroy_debug = mutex_destroy_debug_impl;
}

void uninit_mutex_debug()
{
	mutex_lock_debug = 0;
	mutex_unlock_debug = 0;
	mutex_destroy_debug = 0;
	bt_list.uninit();
}

#endif

void *start_routine2(void *)
{
	ERR("abcd");
	pthread_mutex_t mutex;
	sleep(1);
	pthread_mutex_init(&mutex,NULL);
	//ERR("init value:0x%x",mutex.value);
	pthread_mutex_lock(&mutex); 
	//ERR("start value:0x%x",mutex.value);
	//pthread_mutex_unlock(&mutex); 
	//ERR("unlock value:0x%x",mutex.value);
	pthread_mutex_destroy(&mutex); 
	ERR("end value:0x%x",mutex.value);
	return 0;
}

void *start_routine(void *)
{
	pthread_t thread_out[100];
	for(int i = 0 ; i < 1;i++)
	{
		//pthread_create(&thread_out[i],NULL,start_routine2,NULL);
		//pthread_detach(thread_out[i]);
	}
	for(int i = 0 ; i < 1;i++)
	{
		//pthread_join(thread_out[i],0);
	}
	test_pid = gettid();
	ERR("abcd");
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex,NULL);
	//ERR("init value:0x%x",mutex.value);
	pthread_mutex_lock(&mutex); 
	//ERR("start value:0x%x",mutex.value);
	sleep(1);
	//pthread_mutex_unlock(&mutex); 
	//ERR("unlock value:0x%x",mutex.value);
	//pthread_mutex_destroy(&mutex); 
	ERR("end value:0x%x",mutex.value);
	sleep(1);
	ERR("end exit");
	return 0;
}

void mutex_test()
{
	DEBUG("%s:%d\n",__func__,__LINE__);
	init_mutex_debug();
	//pthread_mutex_t mutex;
	//pthread_mutex_t mutex2;
	//DEBUG("%s:%d\n",__func__,__LINE__);
	//pthread_mutex_init(&mutex,NULL);
	//pthread_mutex_init(&mutex2,NULL);
	//DEBUG("%s:%d\n",__func__,__LINE__);
	//pthread_mutex_lock(&mutex); 
	//DEBUG("%s:%d\n",__func__,__LINE__);
	//dump_all_mutex();
	//pthread_mutex_unlock(&mutex); 
	//DEBUG("%s:%d\n",__func__,__LINE__);
	//dump_all_mutex();
	//DEBUG("%s:%d\n",__func__,__LINE__);
	//pthread_mutex_lock(&mutex); 
	//pthread_mutex_lock(&mutex2); 
	//dump_all_mutex();
	pthread_t thread_out;
	pthread_create(&thread_out,NULL,start_routine,NULL);
	pthread_detach(thread_out);
	//void *aaa = malloc(16);
	//free(aaa);
	ERR("%s:%d-pid:%d\n",__func__,__LINE__,thread_out);
	//pthread_join(thread_out,0);
	for(int i = 0 ; i < 5;i++)
	{
		ERR("dump mutex %d",i);
		dump_all_mutex();
		sleep(1);
	}
	ERR("dump mutex end");
	//pthread_mutex_destroy(&mutex); 
	uninit_mutex_debug();
}
