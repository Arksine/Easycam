#include "easycapture.h"
#include "util.h"
#include "VideoDevice.h"
#include <cstring>



JNIEXPORT jint JNICALL Java_com_arksine_easycam_NativeEasyCapture_startDevice(JNIEnv* jenv, jobject thisObj,
		jstring rsPath, jstring deviceName, jint width, jint height, jint devType,
		jint regionStd, jint numBufs)
{

	char* devName = (char*)jenv->GetStringUTFChars(deviceName, 0);

	DeviceInfo devInfo;

	// Assign basic settings
	CLEAR(devInfo);
	devInfo.device_name = devName;
	devInfo.frame_width = (int)width;
	devInfo.frame_height = (int)height;
	devInfo.device_type = (DeviceType)devType;
	devInfo.standard_id = (VideoStandard)regionStd;
	devInfo.num_buffers = (int)numBufs;

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


	if (vDevice == NULL)
		vDevice = new VideoDevice(devInfo);

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
	fRenderer = new FrameRenderer(jenv, rsPath, devInfo);

	result = vDevice->start_capture();
	if(result != SUCCESS_LOCAL) {
		delete vDevice;
		vDevice = NULL;
		delete fRenderer;
		fRenderer = NULL;
		LOGE("Unable to start capture, resetting device");
	}

	return result;

}
JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_getNextFrame(JNIEnv* jenv, jobject thisObj,
                                                                               jobject surface)
{
	CaptureBuffer * curBuf = NULL;

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
		vDevice = NULL;
	}

	if (fRenderer) {
		delete fRenderer;
		fRenderer = NULL;
	}
}

JNIEXPORT jstring JNICALL Java_com_arksine_easycam_NativeEasyCapture_detectDevice(JNIEnv* jenv, jobject thisObj,
                                                                                  jstring deviceLocation)
{
	char* devName;
	jstring result;

	char* location = (char*)jenv->GetStringUTFChars(deviceLocation, 0);
	devName = VideoDevice::detect_device(location);
	jenv->ReleaseStringUTFChars(deviceLocation, location);

	result = jenv->NewStringUTF(devName);

	return result;
}
