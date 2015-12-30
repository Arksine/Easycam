LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := easycapture
LOCAL_SRC_FILES := easycapture.cpp VideoDevice.cpp util.cpp

# Generate a module that links to /system/lib/liblog.so at load time to enable logging
LOCAL_LDLIBS    := -llog



include $(BUILD_SHARED_LIBRARY)

