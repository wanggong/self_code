# Copyright 2013 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

includes := \
    bionic \
    external/stlport/stlport \

LOCAL_C_INCLUDES += $(includes)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := mutexdebug.cpp 

LOCAL_SHARED_LIBRARIES := libcutils libutils libc libdl libstlport libstdc++ libunwind libbacktrace_libc++

LOCAL_MODULE := libmutexdebug

LOCAL_CFLAGS := -Werror -ggdb

#include $(BUILD_EXECUTABLE)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

includes := \
    bionic \
    external/stlport/stlport \

LOCAL_C_INCLUDES += $(includes)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := mutex_debug_test.cpp

LOCAL_SHARED_LIBRARIES := libcutils libutils libc libdl libstlport libstdc++ libunwind libbacktrace_libc++ libmutexdebug

LOCAL_MODULE := mutex_debug_test

LOCAL_CFLAGS := -Werror -ggdb

include $(BUILD_EXECUTABLE)
#include $(BUILD_SHARED_LIBRARY)


