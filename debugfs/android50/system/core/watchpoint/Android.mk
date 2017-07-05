# Copyright 2013 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := watchpoint.cpp

LOCAL_SHARED_LIBRARIES := libcutils libutils

LOCAL_MODULE := watchpoint

LOCAL_CFLAGS := -Werror

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

include $(BUILD_EXECUTABLE)
