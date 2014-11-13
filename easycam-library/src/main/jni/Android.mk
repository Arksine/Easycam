LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libyuv 
LOCAL_SRC_FILES :=  ./../thirdparty/libyuv/libs/$(TARGET_ARCH_ABI)/libyuv.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := easycam
LOCAL_SRC_FILES := easycam.cpp VideoDevice.cpp util.cpp


# Generate a module that links to /system/lib/liblog.so at load time to enable logging
LOCAL_LDLIBS    := -llog
LOCAL_STATIC_LIBRARIES := libyuv

LOCAL_C_INCLUDES += ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.9/include  \
					$(LOCAL_PATH)/../thirdparty/libyuv/include

include $(BUILD_SHARED_LIBRARY)
