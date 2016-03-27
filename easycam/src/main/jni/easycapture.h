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
#include "FrameRenderer.h"



extern "C" {
    JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_startDevice(JNIEnv* jenv, jobject thisObj,
    		jstring rsPath, jobject dInfo);
    JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_startStreaming(JNIEnv* jenv, jobject thisObj);
    JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_getNextFrame(JNIEnv* jenv, jobject thisObj,
                                                    jobject surface);
    JNIEXPORT jboolean JNICALL Java_com_arksine_easycam_NativeEasyCapture_isDeviceAttached(JNIEnv* jenv, jobject thisObj);
    JNIEXPORT void JNICALL Java_com_arksine_easycam_NativeEasyCapture_stopDevice(JNIEnv* jenv, jobject thisObj);
    JNIEXPORT jstring JNICALL Java_com_arksine_easycam_NativeEasyCapture_detectDevice(JNIEnv* jenv, jclass thisClazz,
                                                       jstring deviceLocation);
};


#endif /* EASYCAM_H_ */
