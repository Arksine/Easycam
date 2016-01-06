//
// Created by Eric on 12/31/2015.
//

#include "FrameRenderer.h"
#include "util.h"

FrameRenderer::FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceInfo devInfo) {

	int bufLength;  // number of bytes in buffer, depends on color space

	frameWidth = devInfo.frame_width;
	frameHeight = devInfo.frame_height;

	//TODO:  Create a setting for deinterlacing, and set the variable accordingly
	//      If no deint, then output height = input height.  Otherwise its half height

	outputFrameHeight = frameHeight / 2;


    // Determine the color space of the input buffer, depending on the device selected
	switch(dSets.color_format){
		case YUYV:
			framePixelFormat = WINDOW_FORMAT_RGBA_8888;
	        processFrame = &FrameRenderer::processFromYUYV;
			initRenderscript(jenv, rsPath);
	        break;
		case UYVY:
	        framePixelFormat = WINDOW_FORMAT_RGBA_8888;
			processFrame = &FrameRenderer::processFromUYVY;
	        initRenderscript(jenv, rsPath);
	        break;
		case RGB565:    // no need to init renderscript here as conversion is not necessary
	        framePixelFormat = WINDOW_FORMAT_RGB_565;
			processFrame = &FrameRenderer::processFromRGB;
	        break;
		case RGBA8888:
	        framePixelFormat = WINDOW_FORMAT_RGBA_8888;
	        processFrame = &FrameRenderer::processFromRGB;
	        break;
		default:
	        framePixelFormat = WINDOW_FORMAT_RGBA_8888;
			processFrame = &FrameRenderer::processFromYUYV;
	        initRenderscript(jenv, rsPath);
	}
}

FrameRenderer::~FrameRenderer() {

}

void FrameRenderer::initRenderscript(JNIEnv* jenv, jstring rsPath) {

	// Determine the size for the input Allocations.  There are 4 bytes in each element,
	// and the input buffer contains 2 bytes per pixel (YUV compression)
	int inAllocationSize = (frameWidth*frameHeight*2) / 4;

	// There is 1 element per pixel, but we are only storing half of the frame in each dimension
	int outAllocationSize = (frameWidth*frameHeight) / 2;

	// get the calling activities path to its Cache Directory, necessary to init renderscript
	const char* path = jenv->GetStringUTFChars(rsPath, NULL);
	rs = new RS();
	rs->init(path);
	jenv->ReleaseStringUTFChars(rsPath, path);

	// TODO:  Rather than calling forEach on the input element, create a dummy allocation
	//        That allows a call on each pixel.  The allocation will contain elements of type int32_t,
	//        and the value of each element will be the y-value of the pixel (so we dont have to calculate
	//        that in the renderscript kernel)

	sp<const Element> inElement = Element::U8_4(rs);
	sp<const Element> outElement = Element::RGBA_8888(rs);
	sp<const Element> pixelElement = Element::I32(rs);


	int xVal = 0;
	int32_t* pixelBuf = new int32_t[outAllocationSize];

	/**
	 * The for loop below calculates the X value of the input allocation when when we are processing
	 * interleaved frames.
	 */
	for (int y=0; i < (frameHeight/2); y++) {

		for (int x=0; j < frameWidth; x++) {
			xVal = (j + (frameWidth*y))/2;
			pixelBuf[offset] = i;
		}
	}
	pixelAlloc.inputAlloc->copy1DFrom((void*)pixelBuf);

	delete [] pixelBuf;

	inputAlloc = Allocation::createSized(rs, inElement, inAllocationSize);
	outputAlloc = Allocation::createSized(rs, outElement, outAllocationSize);
	pixelAlloc = Allocation::createSized(rs, pixelElement, outAllocationSize);

	// create and fill a backing store for the pixel allocation

	script = new ScriptC_convert(rs);

	script->set_inAllocation(inputAlloc);
	script->set_outAllocation(outputAlloc);
	script->set_frameWidth(frameWidth);

}

void FrameRenderer::renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer) {

	ANativeWindow* rWindow = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(rWindow, frameWidth, outputFrameHeight, framePixelFormat);

	// Call the function pointer, which is set in the constructor
	(this->*processFrame)(inBuffer, rWindow);

	ANativeWindow_release(rWindow);
}


void FrameRenderer::processFromYUYV(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(frameWidth/2);  // Bob Even first
	script->forEach_convertFieldFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

	script->set_firstElement(0);
	script->forEach_convertFieldFromYUYV(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}



void FrameRenderer::processFromUYVY(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(frameWidth/2);  // Bob Even first
	script->forEach_convertFieldFromUYVY(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

	script->set_firstElement(0);
	script->forEach_convertFieldFromUYVY(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}

void FrameRenderer::processFromRGB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	//TODO: If bob deinterlacing works then this wont, needs to be fixed

	// Write buffer directly to window, no conversion is necessary
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		memcpy(wBuffer.bits, inBuffer->start,  inBuffer->length);
		ANativeWindow_unlockAndPost(window);
	}
}