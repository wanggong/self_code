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
//#define DEBUG(fmt, args...) do{}while(0)
#define DEBUG(fmt, args...) ALOGE("MUTEX "fmt, ##args)

extern void mutex_test();

int main()
{
	//c();
	DEBUG("%s:%d\n",__func__,__LINE__);
	//thread_mutex_test();
	int a = 90;
	DEBUG("a=%d\n" ,a);
	//dl_iterate_phdr_unwind(0,&a); 
	DEBUG("a=%d\n" ,a);
	mutex_test();
	return 0;
}
