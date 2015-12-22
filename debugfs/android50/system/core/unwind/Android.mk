# Copyright 2013 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

libbacktrace_libc++_c_includes := \
	external/libunwind/include \

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := unwind.cpp

LOCAL_SHARED_LIBRARIES := libcutils libutils libc

LOCAL_MODULE := unwind

LOCAL_CFLAGS := -Werror 

include $(BUILD_EXECUTABLE)
