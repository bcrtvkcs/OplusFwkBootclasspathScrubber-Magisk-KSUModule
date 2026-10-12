LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := oplusfwk_scrubber
LOCAL_SRC_FILES := main.cpp
LOCAL_CPPFLAGS := -std=c++17 -Wall -Wextra -O3 -fvisibility=hidden -fvisibility-inlines-hidden
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
