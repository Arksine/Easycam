#include "easycapture.h"
#include "util.h"
#include "VideoDevice.h"
#include <cstring>



JNIEXPORT jint JNICALL Java_com_arksine_easycam_NativeEasyCapture_startDevice(JNIEnv* jenv, jobject thisObj,
		jstring rsPath, jstring deviceName, jint width, jint height, jint devType,
		jint regionStd, jint numBufs)
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

	// TODO: retreive pixel format from Java settings, rather than set it here
	switch(dSets.device_type) {
		case UTV007:        //YUYV
		case EMPIA:         //YUYV
			dSets.color_format = YUYV;
			break;
		case STK1160:       //UYVY
		case SOMAGIC:       //UYVY
			dSets.color_format = UYVY;
			break;
		case CUSTOM:        //Placeholder for custom format
			dSets.color_format = RGB565;
			break;
		default:            //Generic, assume YUYV
			dSets.color_format = YUYV;
			break;
	}


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

	// instantiate frame renderer only if the video device is successfully initialized
	fRenderer = new FrameRenderer(jenv, rsPath, vDevice->get_buffer_length(),
								  dSets.color_format);

	result = vDevice->start_capture();
	if(result != SUCCESS_LOCAL) {
		delete vDevice;
		vDevice = nullptr;
		delete fRenderer;
		fRenderer = nullptr;
		LOGE("Unable to start capture, resetting device");
	}

	return result;

}
JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_getNextFrame(JNIEnv* jenv, jobject thisObj,
                                                                               jobject surface)
{
	CaptureBuffer * curBuf = nullptr;

	if (vDevice)
		curBuf = vDevice->process_capture();


	if (curBuf)
		fRenderer->renderFrame(jenv, surface, curBuf);

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
JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_stopDevice(JNIEnv* jenv, jobject thisObj) {
	if (vDevice) {
		delete vDevice;
		vDevice = nullptr;
	}

	if (fRenderer) {
		delete fRenderer;
		fRenderer = nullptr;
	}
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
