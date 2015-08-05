LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := android_backtrace.c
LOCAL_MODULE := libandroid_backtrace
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcorkscrew liblog
include $(BUILD_SHARED_LIBRARY)
