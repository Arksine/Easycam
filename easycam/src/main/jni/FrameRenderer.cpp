//
// Created by Eric on 12/31/2015.
//

#include "FrameRenderer.h"
#include "util.h"

FrameRenderer::FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceSettings dSets) {

	int bufLength;  // number of bytes in buffer, depends on color space

	frameWidth = dSets.frame_width;
	frameHeight = dSets.frame_height;

	// output frame is 4 bytes per pixel, but half of the frame height as we are bob deinterlacing
	outputFrameSize = (frameWidth * frameHeight * 4) / 2;


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
	int outAllocationWidth = (frameWidth*frameHeight) / 2;

	int inputXElements = (frameWidth) * 2 / 4;

	// get the calling activities path to its Cache Directory, necessary to init renderscript
	const char* path = jenv->GetStringUTFChars(rsPath, NULL);
	rs = new RS();
	rs->init(path);
	jenv->ReleaseStringUTFChars(rsPath, path);

	sp<const Element> inElement = Element::U8_4(rs);
	sp<const Element> outElement = Element::RGBA_8888(rs);


	inputAlloc = Allocation::createSized(rs, inElement, inAllocationSize);
	outputAlloc = Allocation::createSized2D(rs, outElement, outAllocationWidth, 2);

	script = new ScriptC_convert(rs);

	script->set_output(outputAlloc);
	script->set_xElements(inputXElements);

}

void FrameRenderer::renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer) {

	ANativeWindow* rWindow = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(rWindow, frameWidth, (frameHeight / 2), framePixelFormat);

	// Call the function pointer, which is set in the constructor
	(this->*processFrame)(inBuffer, rWindow);

	ANativeWindow_release(rWindow);
}


void FrameRenderer::processFromYUYV(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);
	script->forEach_convertFromYUYV(inputAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, odd first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy2DRangeTo(0, 0, outputFrameSize , 0, wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy2DRangeTo(0, 0, outputFrameSize , 1, wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}



void FrameRenderer::processFromUYVY(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);
	script->forEach_convertFromUYVY(inputAlloc);

	// TODO:  Add output frame size to private data memebers, calculate in constructor

	// Write output buffers to the window, we are attempting bob deinterlacing, odd first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy2DRangeTo(0, 0, outputFrameSize , 0, wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy2DRangeTo(0, 0, outputFrameSize , 1, wBuffer.bits);
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