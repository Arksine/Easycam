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
     FrameRenderer(JNIEnv* jenv, jobject surface, jstring rsPath, int bufLength,
                    PixelFormat pFormat);
    ~FrameRenderer();

    void renderFrame(CaptureBuffer* inBuffer);

private:
    ANativeWindow* window;

	sp<RS> rs;
    sp<Allocation> inputAlloc;
    sp<Allocation> outputAlloc;

    ScriptC_convert* script;

	void initRenderscript(JNIEnv* jenv, jstring rsPath, int bufLength);

    // Function pointer to store which call we need to make, depending on YUV type
    void (FrameRenderer::* processFrame)(CaptureBuffer*);

    void processFromYUYV(CaptureBuffer* inBuffer);
    void processFromUYVY(CaptureBuffer* inBuffer);
    void processFromRGB(CaptureBuffer* inBuffer);

};


#endif //EASYCAM_FRAMERENDERER_H
