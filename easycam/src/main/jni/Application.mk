NDK_TOOLCHAIN_VERSION := 4.9
APP_ABI := armeabi-v7a x86
#  Enable C++11. However, pthread, rtti and exceptions arent enabled
APP_CPPFLAGS += -std=c++11
APP_PLATFORM := android-19
APP_STL := stlport_static
