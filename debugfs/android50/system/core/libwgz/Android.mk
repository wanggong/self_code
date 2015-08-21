LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := wgz_debug.cpp
LOCAL_MODULE := libwgz_debug
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils 
include $(BUILD_SHARED_LIBRARY)
