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
	FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceSettings* dSets);
    ~FrameRenderer();

	void renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer);

private:

	/**
	 * TODO: Add functionality to choose deinterlacing options(none, discard, bob even first,
	 *       bob odd first, and perhaps blend.  Will need to choose options from settings in
	 *       java.
	 */

    DeviceSettings* devSettings;
	int outputFrameHeight;  
    
    

    // TODO: Need to set these variables in the constructor
    // These are used for bob and discard deinterlacing.  They tell the renderscript kernels
    // which element is the beginning of the first frame and which Element is the beginning of the
    // second frame.
    int firstFrameElementIndex;
    int secondFrameElementIndex;
	
	int32_t frameWindowFormat;  // The pixel format used by ANativeWindow

	sp<RS> rs;
    sp<Allocation> inputAlloc;
    sp<Allocation> outputAlloc;
    sp<Allocation> pixelAlloc;		// Allocation that represents each pixel in the input buffer
	sp<Allocation> intrinsAlloc;	// Allocation that the YUV Intrinsic writes to.

    ScriptC_convert* script;
	sp<ScriptIntrinsicYuvToRGB> intrinsic;

	// TODO:  We need multiple init Renderscript functions.  The initilization depends on if/how
	//        we deinterlace (ie the size of the allocations differ)
	void initRenderscript(int inputFrameHeight, bool interleaved, RSYuvFormat yuvFmt);
	void initYuvIntrinsic(RSYuvFormat yuvFmt, int inputFrameHeight, int intrinsFrameHeight);

    // Function pointer to store which call we need to make, depending on YUV type
    void (FrameRenderer::* processFrame)(CaptureBuffer*, ANativeWindow*);

	// Function pointer to the renderscript kernel function that needs to be called
	// while processing
	void (ScriptC_convert::* executeKernel)(sp<const Allocation>);

    void processRS_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processRS_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processRS_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processRS_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);   // TODO: STUB

    // RGB needs no color conversion, and with no deinterlacing it needs no calls to renderscript
    void processRGB_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);

    // The below is for NV21 and YV12, which use YUV intrinsic
    void processIntrinsic_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processIntrinsic_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processIntrinsic_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processIntrinsic_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);  // TODO: STUB

    // TODO: If the above function pointer works correctly there is no need for most of these
    //       functions, as their contents only differ by the kernel called.  Commenting out for now
    /*
    // YUYV functions.  The label after the underscore represents the kind of deinterlacing
    void processYUYV_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processYUYV_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processYUYV_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    //void processYUYV_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);

    // UYVY functions
    void processUYVY_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processUYVY_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processUYVY_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    //void processUYVY_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);

    // NV21 functions
    void processNV21_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processNV21_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processNV21_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    //void processNV21_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);

    // YV12 functions
    void processYV12_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processYV12_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processYV12_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    //void processYV12_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window);


    // RGB functions
    void processRGB_NONE(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processRGB_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window);
    void processRGB_BOB(CaptureBuffer* inBuffer, ANativeWindow* window);
    //void processRGB_BLEND(CaptureBuffer* inBuffer, ANativeWindow* window); */
};


#endif //EASYCAM_FRAMERENDERER_H
