/*
 * easycam.h
 *
 *  Created on: Nov 6, 2014
 *      Author: Eric
 */

#ifndef EASYCAM_H_
#define EASYCAM_H_

#include <jni.h>
#include "VideoDevice.h"

VideoDevice* vDevice = nullptr;

extern "C" {
    JNIEXPORT jint JNICALL Java_com_arksine_easycamlib_NativeEasycam_startDevice(JNIEnv* jenv, jobject thisObj,
    		jobject rgbBuf, jstring deviceName, jint width, jint height, jint devType, jint regionStd, jint numBufs);
    JNIEXPORT void JNICALL Java_com_arksine_easycamlib_NativeEasycam_getNextFrame(JNIEnv* jenv, jobject thisObj,
    		jint bmpWidth, jint bmpHeight);
    JNIEXPORT jboolean JNICALL Java_com_arksine_easycamlib_NativeEasycam_isDeviceAttached(JNIEnv* jenv, jobject thisObj);
    JNIEXPORT void JNICALL Java_com_arksine_easycamlib_NativeEasycam_stopDevice(JNIEnv* jenv, jobject thisObj);
    JNIEXPORT jstring JNICALL Java_com_arksine_easycamlib_NativeEasycam_detectDevice(JNIEnv* jenv, jobject thisObj, jstring deviceName);
};


#endif /* EASYCAM_H_ */
