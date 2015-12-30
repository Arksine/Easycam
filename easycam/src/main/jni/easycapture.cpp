#include "easycapture.h"
#include "util.h"
#include "VideoDevice.h"
#include <cstring>

jbyteArray capBuffer;

JNIEXPORT jint JNICALL Java_com_arksine_easycam_NativeEasyCapture_startDevice(JNIEnv* jenv, jobject thisObj,
		jstring deviceName, jint width, jint height, jint devType, jint regionStd, jint numBufs)
{

	char* devName = (char*)jenv->GetStringUTFChars(deviceName, 0);

	DeviceSettings dSets;

	// Assign basic settings
	CLEAR(dSets);
	dSets.device_name = devName;
	dSets.frame_width = (int)width;
	dSets.frame_height = (int)height;
	dSets.device_type = (DeviceType)devType;
	dSets.standard_id = (VideoStandard)regionStd;
	dSets.num_buffers = (int)numBufs;


	if (vDevice == nullptr)
		vDevice = new VideoDevice(dSets);

	int result = vDevice->open_device();
	jenv->ReleaseStringUTFChars(deviceName, devName);
	if(result == ERROR_LOCAL) {
	        return result;
	}

	result = vDevice->init_device();
	if(result == ERROR_LOCAL) {
		return result;
	}

    // allocate the buffer to send back to Java
    capBuffer = jenv->NewByteArray(vDevice->get_buffer_length());

	result = vDevice->start_capture();
	if(result != SUCCESS_LOCAL) {
	        delete vDevice;
	        vDevice = nullptr;
	        LOGE("Unable to start capture, resetting device");
	}

	return result;

}
JNIEXPORT jbyteArray JNICALL Java_com_arksine_easycam_NativeEasyCapture_getNextFrame(JNIEnv* jenv, jobject thisObj)
{
	buffer* curBuf = NULL;

	if (vDevice) {
		curBuf = vDevice->process_capture();
	}

	jenv->SetByteArrayRegion(capBuffer, 0, curBuf->length, (jbyte*)(curBuf->start));

    return  capBuffer;
}
JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_isDeviceAttached(JNIEnv* jenv, jobject thisObj)
{
	if (vDevice) {
		return vDevice->video_device_attached();
	}
	else {
//		LOGE("Video Device Not Initialized");
		return false;
	}
}
JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_stopDevice(JNIEnv* jenv, jobject thisObj)
{

	delete vDevice;
	vDevice = nullptr;
    jenv->DeleteLocalRef(capBuffer);
}

JNIEXPORT jstring JNICALL Java_com_arksine_easycam_NativeEasyCapture_detectDevice(JNIEnv* jenv, jobject thisObj, jstring deviceName)
{
	DeviceType dType = NO_DEVICE;
	jstring result;

	char* devName = (char*)jenv->GetStringUTFChars(deviceName, 0);
	dType = VideoDevice::detect_device(devName);
	jenv->ReleaseStringUTFChars(deviceName, devName);

	switch(dType) {
	case UTV007:
		result = jenv->NewStringUTF("UTV007");
		break;
	case EMPIA:
		result = jenv->NewStringUTF("EMPIA");
		break;
	case STK1160:
		result = jenv->NewStringUTF("STK1160");
		break;
	case SOMAGIC:
		result = jenv->NewStringUTF("SOMAGIC");
		break;
	default:
		result = jenv->NewStringUTF("NODEVICE");
	}

	return result;
}
