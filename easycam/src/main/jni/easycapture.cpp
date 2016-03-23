#include "easycapture.h"
#include "util.h"
#include "VideoDevice.h"
#include <cstring>

void initializeSettings(JNIEnv* jenv, jobject dInfo, DeviceSettings* devSets);

VideoDevice* vDevice = NULL;
FrameRenderer* fRenderer = NULL;
DeviceSettings* devSettings = NULL;

JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_startDevice(JNIEnv* jenv, jobject thisObj,
		jstring rsPath, jobject dInfo)
{

	if (devSettings == NULL)
		devSettings = new DeviceSettings;

	CLEAR(*devSettings);
	initializeSettings(jenv, dInfo, devSettings);

	if (vDevice == NULL)
		vDevice = new VideoDevice(devSettings);

	int result = vDevice->openDevice();
	if(result == ERROR_LOCAL) {
		LOGE("Unable to open device");
		return false;
	}

	result = vDevice->initDevice();
	if(result == ERROR_LOCAL) {
		LOGE("Unable to initialize device");
		return false;
	}

	// instantiate frame renderer after video device is successfully initialized
	if (fRenderer == NULL)
		fRenderer = new FrameRenderer(jenv, rsPath, devSettings);

	return true;

}

JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_startStreaming(JNIEnv* jenv, jobject thisObj) {

	int result = vDevice->startCapture();

	if(result != SUCCESS_LOCAL) {
		delete vDevice;
		vDevice = NULL;
		delete fRenderer;
		fRenderer = NULL;
		LOGE("Unable to start capture, resetting device");

		return false;
	}

	return true;
}

JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_getNextFrame(JNIEnv* jenv, jobject thisObj,
                                                                               jobject surface)
{
	CaptureBuffer * curBuf = NULL;

	if (vDevice)
		curBuf = vDevice->processCapture();


	if (curBuf)
		fRenderer->renderFrame(jenv, surface, curBuf);

}
JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_isDeviceAttached(JNIEnv* jenv, jobject thisObj)
{
	if (vDevice) {
		return vDevice->videoDeviceAttached();
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

	if (devSettings) {
		delete devSettings;
		devSettings = NULL;
	}
}

JNIEXPORT jstring JNICALL Java_com_arksine_easycam_NativeEasyCapture_detectDevice(JNIEnv* jenv, jclass thisClazz,
                                                                                  jstring deviceLocation)
{
	char* devName;
	jstring result;

	char* location = (char*)jenv->GetStringUTFChars(deviceLocation, 0);
	devName = VideoDevice::detectDevice(location);
	jenv->ReleaseStringUTFChars(deviceLocation, location);

	result = jenv->NewStringUTF(devName);

	return result;
}

/**
 * Takes the device information passed from the calling java object and applies
 * it to our DeviceSettings C struct.
 */
void initializeSettings(JNIEnv* jenv, jobject dInfo, DeviceSettings* devSets) {

	// TODO: 3/22/2016
	// Add Usb Vendor ID and Usb Product ID if we think we need them.
	jclass devInfo = jenv->GetObjectClass(dInfo);
	jmethodID getDriver = jenv->GetMethodID(devInfo, "getDriver", "()Ljava/lang/String");
	jmethodID getLocation = jenv->GetMethodID(devInfo, "getLocation","()Ljava/lang/String" );
	jmethodID getFrameWidth = jenv->GetMethodID(devInfo, "getFrameWidth","()I");
	jmethodID getFrameHeight = jenv->GetMethodID(devInfo, "getFrameHeight","()I");
	jmethodID getNumBuffers = jenv->GetMethodID(devInfo, "getNumBuffers","()I");
	jmethodID getInput = jenv->GetMethodID(devInfo, "getInput", "()I");
	jmethodID getDevStandard = jenv->GetMethodID(devInfo, "getDevStandard",
	                                             "()Lcom/arksine/easycam/DeviceInfo$DeviceStandard");
	jmethodID getPixFmt = jenv->GetMethodID(devInfo, "getPixFmt",
	                                        "()Lcom/arksine/easycam/DeviceInfo$PixelFormat");
	jmethodID getDeinterlace = jenv->GetMethodID(devInfo, "getDeinterlace",
	                                             "()Lcom/arksine/easycam/DeviceInfo$DeintMethod");
	jmethodID getFieldType = jenv->GetMethodID(devInfo, "getFieldType",
	                                      "()Lcom/arksine/easycam/DeviceInfo$FieldType");

	// Get the enumerations first, as they also have methods well need to get
	jobject devStd = jenv->CallObjectMethod(dInfo, getDevStandard);
	jobject pixFmt = jenv->CallObjectMethod(dInfo, getPixFmt);
	jobject deint = jenv->CallObjectMethod(dInfo, getDeinterlace);
	jobject field = jenv->CallObjectMethod(dInfo, getFieldType);

	jclass standard = jenv->GetObjectClass(devStd);
	jmethodID getStdIndex =jenv->GetMethodID(standard, "getIndex", "()I");

	jclass pixelFormat = jenv->GetObjectClass(pixFmt);
	jmethodID getPixFmtIndex = jenv->GetMethodID(pixelFormat, "getIndex", "()I");

	jclass deinterlace = jenv->GetObjectClass(deint);
	jmethodID getDeintIndex = jenv->GetMethodID(deinterlace, "getIndex", "()I");

	jclass fieldType = jenv->GetObjectClass(field);
	jmethodID getFieldIndex = jenv->GetMethodID(fieldType, "getIndex", "()I");

	// TODO:  Need to be absolutely sure that when we release UTF Chars we aren't deleting the string
	//        in the device settings
	jstring tmpStr = (jstring)jenv->CallObjectMethod(dInfo, getDriver);
	devSets->driver = (char*)jenv->GetStringUTFChars(tmpStr, 0);
	jenv->ReleaseStringUTFChars(tmpStr, devSets->driver);

	tmpStr = (jstring)jenv->CallObjectMethod(dInfo, getLocation);
	devSets->location = (char*)jenv->GetStringUTFChars(tmpStr, 0);
	jenv->ReleaseStringUTFChars(tmpStr, devSets->location);

	devSets->frameWidth = (int)jenv->CallIntMethod(dInfo, getFrameWidth);
	devSets->frameHeight = (int)jenv->CallIntMethod(dInfo, getFrameHeight);
	devSets->numBuffers = (int)jenv->CallIntMethod(dInfo, getNumBuffers);
	devSets->input = (int)jenv->CallIntMethod(dInfo, getInput);

	int tmpIndex = 0;

	// Set Video Standard
	tmpIndex = (int)jenv->CallIntMethod(devStd, getStdIndex);
	switch (tmpIndex) {
		case 0:     // NTSC
			devSets->videoStandard = V4L2_STD_NTSC;
			break;
		case 1:
			devSets->videoStandard = V4L2_STD_PAL;
	        break;
		default:    // PAL
			//Should not happen
			devSets->videoStandard = V4L2_STD_NTSC;
			LOGE("Problem setting Video Standard");
			break;
	}

	// Set Pixel Format
	tmpIndex = (int)jenv->CallIntMethod(pixFmt, getPixFmtIndex);
	switch (tmpIndex) {
		case 0:     // YUYV
			devSets->pixelFormat = V4L2_PIX_FMT_YUYV;
			break;
		case 1:     // UYVY
			devSets->pixelFormat = V4L2_PIX_FMT_UYVY;
			break;
		case 2:     // NV21
			devSets->pixelFormat = V4L2_PIX_FMT_NV21;
			break;
		case 3:     // YV12
			devSets->pixelFormat = V4L2_PIX_FMT_YVU420;
			break;
		case 4:     // RGB565
			devSets->pixelFormat = V4L2_PIX_FMT_RGB565;
			break;
		default:
			LOGE("Problem setting Pixel Format");
			devSets->pixelFormat = V4L2_PIX_FMT_YUYV;
			break;
	}

	// Set Deinterlace Method
	tmpIndex = (int)jenv->CallIntMethod(deint, getDeintIndex);
	switch (tmpIndex) {
		case 0:     // NONE
			devSets->deintMethod = NONE;
			break;
		case 1:     // DISCARD
			devSets->deintMethod = DISCARD;
			break;
		case 2:     // BOB
			devSets->deintMethod = BOB;
			break;
		case 3:     // BLEND
			devSets->deintMethod = BLEND;
			break;
		default:
			LOGE("Problem setting Deinterlace Method");
			devSets->deintMethod = NONE;
			break;
	}

	// Set Field Type
	tmpIndex = (int)jenv->CallIntMethod(field, getFieldIndex);
	switch (tmpIndex) {
		case 0:     // NONE (Progressive Scan)
			devSets->field = V4L2_FIELD_NONE;
			break;
		case 1:     // TOP_ONLY
			devSets->field = V4L2_FIELD_TOP;
			break;
		case 2:     // BOTTOM_ONLY
			devSets->field = V4L2_FIELD_BOTTOM;
			break;
		case 3:     // INTERLACED
			devSets->field = V4L2_FIELD_INTERLACED;
			break;
		case 4:     // SEQUENTIAL_TB
			devSets->field = V4L2_FIELD_SEQ_TB;
			break;
		case 5:     // SEQUENTIAL_BT
			devSets->field = V4L2_FIELD_SEQ_BT;
			break;
		case 6:     // ALTERNATE
			devSets->field = V4L2_FIELD_ALTERNATE;
			break;
		default:
			LOGE("Problem setting Field Type");
			devSets->field = V4L2_FIELD_INTERLACED;
			break;

	}

}