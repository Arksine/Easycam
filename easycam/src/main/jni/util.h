#ifndef __UTIL__H__
#define __UTIL__H__

#include <jni.h>
#include <cstdio>
#include <sys/ioctl.h>
#include <android/log.h>

#define LOG_TAG "NativeEasyCaptureJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define ERROR_LOCAL -1
#define SUCCESS_LOCAL 0

// Simple buffer that stores a reference to an array and the length of the array
typedef struct {
    void* start;
    size_t length;
} CaptureBuffer;

// Enumerations for tracking device info
enum DeviceType {UTV007, EMPIA, STK1160, SOMAGIC, CUSTOM, NO_DEVICE};
enum PixelFormat {YUYV, UYVY, RGB565, RGBA8888};
enum VideoStandard {NTSC, PAL};

typedef struct {
	DeviceType device_type;     // Type of easycap device
	VideoStandard standard_id;  // Region standard (NTSC/PAL)
	int frame_width;
	int frame_height;
	char* device_name;  		// location of the device file (/dev/videoX)
	int num_buffers;			// number of buffers to allocate
	PixelFormat color_format;   // the pixel format of the device
} DeviceSettings;

int errnoexit(const char *s);

/* Private: Repeat an ioctl call until it completes and is not interrupted by a
 * a signal.
 *
 * The ioctl may still succeed or fail, so do check the return status.
 *
 * fd - the file descriptor for the ioctl.
 * request - the type of IOCTL to request.
 * arg - the target argument for the ioctl.
 *
 * Returns the status of the ioctl when it completes.
 */
int xioctl(int fd, int request, void *arg);

#endif // __UTIL__H__
