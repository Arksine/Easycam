//
// Created by Eric on 12/31/2015.
//

#ifndef EASYCAM_FRAMERENDERER_H
#define EASYCAM_FRAMERENDERER_H

#include "util.h"
#include <android/native_window_jni.h>
#include <RenderScript.h>
#include "ScriptC_convert.h"

using namespace android::RSC;
/**
 * Class: FrameRenderer
 *        This class performs all color conversion (through renderscript) and output
 *        to the applications window
 */
class FrameRenderer {
public:
     FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceInfo devInfo);
    ~FrameRenderer();

    void renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer);

private:

	/**
	 * TODO: Add functionality to choose deinterlacing options(none, discard, bob even first,
	 *       bob odd first, and perhaps blend.  Will need to choose options from settings in
	 *       java.
	 */

    int outputFrameHeight;
	int frameWidth;        // input frame width in pixels
	int frameHeight;       // input frame height in pixels
	int32_t framePixelFormat;

	sp<RS> rs;
    sp<Allocation> inputAlloc;
    sp<Allocation> outputAlloc;
    sp<Allocation> pixelAlloc;   // Allocation that represents each pixel in the input buffer

    ScriptC_convert* script;

	void initRenderscript(JNIEnv* jenv, jstring rsPath);

    // Function pointer to store which call we need to make, depending on YUV type
    void (FrameRenderer::* processFrame)(CaptureBuffer*, ANativeWindow*);

    void processFromYUYV(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processFromUYVY(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processFromRGB(CaptureBuffer* inBuffer, ANativeWindow* window);

};


#endif //EASYCAM_FRAMERENDERER_H
