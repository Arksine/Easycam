LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := easycapture
LOCAL_SRC_FILES := easycapture.cpp VideoDevice.cpp util.cpp FrameRenderer.cpp convert.rs

LOCAL_LDFLAGS += -L$(call host-path,$(TARGET_C_INCLUDES)/../lib/rs)

# Generate a module that links to /system/lib/liblog.so at load time to enable logging
LOCAL_LDLIBS    := -llog \
                   -landroid \
                   -lRScpp_static

LOCAL_C_INCLUDES += $(TARGET_C_INCLUDES)/rs/cpp
LOCAL_C_INCLUDES += $(TARGET_C_INCLUDES)/rs
LOCAL_C_INCLUDES += $(TARGET_OBJS)/$(LOCAL_MODULE)

include $(BUILD_SHARED_LIBRARY)

