//
// Created by Eric on 12/31/2015.
//

#include "FrameRenderer.h"
#include "util.h"

#define INITRS(process, forEach)					\
	initRenderscript(jenv, rsPath);					\
	processFrame = &FrameRenderer:: ## process;		\
	executeKernel = &ScriptC_convert:: ## forEach;

/*
*	deint - deinterlacing method.  IE: NONE, DISCARD, BOB, BLEND.
*	ft - field type.  Typically Frame, it will be Field if we are doing deinterlacing on an interleaved frame
*/
#define COLORSWITCH(method, ft)													\
	switch(devSettings->pixelFormat){											\
		case V4L2_PIX_FMT_YUYV:													\
			INITRS(processRS_ ## method, forEach_convert ## ft ## FromYUYV)		\
			break;																\
		case V4L2_PIX_FMT_UYVY:													\
			INITRS(processRS_ ## method, forEach_convert ## ft ## FromUYVY)		\
			break;																\
		case V4L2_PIX_FMT_NV21:													\
			break;																\
		case V4L2_PIX_FMT_YVU420:												\
			break;																\
		case V4L2_PIX_FMT_RGB565:												\
			if (strncmp(#method, "NONE", 4) == 0) {								\
				processFrame = &FrameRenderer::processRGB_NONE;					\
			}																	\
			else {																\
				INITRS(processRS_ ## method, forEach_separate ## ft ## FromRGB)	\
			}																	\
			break;																\
		default:																\
			INITRS(process ## method, ft ## FromYUYV)							\
	}



//TODO:  Create a renderer using scriptIntrisincYUVtoRGB for NV21 and YV12 pixel formats

FrameRenderer::FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceSettings* dSets) {

	devSettings = dSets;
	int bufLength;  // number of bytes in buffer, depends on color space

	//TODO:  We need to create functions and set them to function pointers to support
	//       the various types of deinterlacing.  The FieldType also affects deinterlacing
	//       and rendering, so functionality to account for that is necessary as well.

	// Set the frame output pixel format, which will always be RGBA8888 unless the
	// input colorspace is RGB565
	if (devSettings->pixelFormat == V4L2_PIX_FMT_RGB565) {
		framePixelFormat = WINDOW_FORMAT_RGB_565;
	}
	else {
		framePixelFormat = WINDOW_FORMAT_RGBA_8888;
	}

	switch(devSettings->field) {
		case V4L2_FIELD_NONE:	// Progressive Scan, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			outputFrameHeight = devSettings->frameHeight;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;   // not used
			// init renderscript, color conversion only
			break;
		case V4L2_FIELD_TOP:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputFrameHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			// init renderscript, color conversion only
			break;
		case V4L2_FIELD_BOTTOM:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputFrameHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			// init renderscript, color conversion only
			break;
		case V4L2_FIELD_INTERLACED:
			interleaved = true;
			inputFrameHeight = devSettings->frameHeight;
			switch (devSettings->deintMethod) {
				case NONE:
					outputFrameHeight = devSettings->frameHeight;
					firstFrameElementIndex = 0;
					secondFrameElementIndex = 0;
					switch(devSettings->pixelFormat){
						case V4L2_PIX_FMT_YUYV:
							framePixelFormat = WINDOW_FORMAT_RGBA_8888;
							//processFrame = &FrameRenderer::processYUYV_NONE;
							//initRenderscript(jenv, rsPath);
							//executeKernel = &ScriptC_convert::forEach_convertFrameFromYUYV;
							INITRS(processRS_NONE, forEach_convertFrameFromYUYV)
							break;
						case V4L2_PIX_FMT_UYVY:
							framePixelFormat = WINDOW_FORMAT_RGBA_8888;
							processFrame = &FrameRenderer::processUYVY_NONE;
							initRenderscript(jenv, rsPath);
							break;
						case V4L2_PIX_FMT_NV21:
							framePixelFormat = WINDOW_FORMAT_RGBA_8888;
							processFrame = &FrameRenderer::processNV21_NONE;
							break;
						case V4L2_PIX_FMT_YVU420:
							framePixelFormat = WINDOW_FORMAT_RGBA_8888;
							processFrame = &FrameRenderer::processYV12_NONE;
							break;
						case V4L2_PIX_FMT_RGB565:
							// no need to init renderscript here as conversion is not necessary
							framePixelFormat = WINDOW_FORMAT_RGB_565;
							processFrame = &FrameRenderer::processRGB_NONE;
							break;
						default:
							framePixelFormat = WINDOW_FORMAT_RGBA_8888;
							processFrame = &FrameRenderer::processYUYV_NONE;
							initRenderscript(jenv, rsPath);
					}
					break;
				case DISCARD:
					outputFrameHeight = devSettings->frameHeight / 2;
					if (devSettings->videoStandard == V4L2_STD_NTSC) {	
						firstFrameElementIndex = devSettings->frameWidth / 2;
						secondFrameElementIndex = 0;
					}
					else {
						firstFrameElementIndex = 0;
						secondFrameElementIndex = devSettings->frameWidth / 2;
					}
					break;
				case BOB:
					outputFrameHeight = devSettings->frameHeight / 2;
					if (devSettings->videoStandard == V4L2_STD_NTSC) {	
						firstFrameElementIndex = devSettings->frameWidth / 2;
						secondFrameElementIndex = 0;
					}
					else {
						firstFrameElementIndex = 0;
						secondFrameElementIndex = devSettings->frameWidth / 2;
					}

					break;
				//case BLEND:
					//outputFrameHeight = devSettings->frameHeight;
					//break;
				default:
					break;			
			}
			break;
		case V4L2_FIELD_SEQ_TB:
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			// TODO: Move if statement below into the switch statement under discard and bob
			if (devSettings->videoStandard == V4L2_STD_NTSC) {	
				firstFrameElementIndex = (devSettings->frameWidth*devSettings->frameHeight) / 4;
				secondFrameElementIndex = 0;
			}
			else {
				firstFrameElementIndex = 0;
				secondFrameElementIndex = (devSettings->frameWidth*devSettings->frameHeight) / 4;
			}
			switch (devSettings->deintMethod) {
			
			}
			break;
		case V4L2_FIELD_SEQ_BT:
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			switch (devSettings->deintMethod) {
			
			}
			break;
		case V4L2_FIELD_ALTERNATE:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputFrameHeight = devSettings->frameHeight / 2;
			break;
		default:
			break;
	}

}

FrameRenderer::~FrameRenderer() {

}

void FrameRenderer::initRenderscript(JNIEnv* jenv, jstring rsPath) {

	int inAllocWidth = devSettings->frameWidth / 2;  // The input allocation has half the number of elements as there are pixels

	// TODO:  The outAllocWidth is half width when dealing with RGB565
	int outAllocWidth = devSettings->frameWidth;     // number of output elements = number of pixels

	// Determine the size for the input Allocations.  There are 4 bytes in each element,
	// and the input buffer contains 2 bytes per pixel (YUV compression)
	int inAllocationSize = inAllocWidth * inputFrameHeight;

	// There is 1 element per pixel, but we are only storing half of the frame in each dimension
	int outAllocationSize = outAllocWidth * outputFrameHeight;

	// get the calling activities path to its Cache Directory, necessary to init renderscript
	const char* path = jenv->GetStringUTFChars(rsPath, NULL);
	rs = new RS();
	rs->init(path);
	jenv->ReleaseStringUTFChars(rsPath, path);


	sp<const Element> inElement = Element::U8_4(rs);
	sp<const Element> outElement = Element::RGBA_8888(rs);
	sp<const Element> pixelElement = Element::I32(rs);

	// create and fill a backing store for the pixel allocation
	int xVal = 0;
	int32_t* pixelBuf = new int32_t[outAllocationSize];

	/**
	 * The for loop below calculates the X value of the input allocation.
	 *
	 * TODO: The for loops below only work for 16-bit input formats, however we aren't currently allowing 
	 *	     input formats containing 24 or 32 bits per pixel.  If I change that I probably don't need
	 *		 a pixel allocation anyway, as they are already working on one element per pixel.
	 *  
	 */
	if (interleaved) {
		// Calculate the x values for the input allocation in interleaved fields
		for (int y=0; y < (outputFrameHeight/2); y++) {

			for (int x=0; x < outAllocWidth; x++) {
				xVal = (x + ((2*outAllocWidth)*y))/2; 			
				pixelBuf[x + (y*outAllocWidth)] = xVal;
			}
		}
		
	}
	else {
		// Calculate the x values for the input allocation in sequential fields
		for (int y=0; y < outputFrameHeight; y++) {

			for (int x=0; x < outAllocWidth; x++) {
				xVal = (x + (outAllocWidth*y))/2; 
				pixelBuf[x + (y*outAllocWidth)] = xVal;
			}
		}
		
	}

	pixelAlloc->copy1DFrom((void*)pixelBuf);

	delete [] pixelBuf;

	

	inputAlloc = Allocation::createSized(rs, inElement, inAllocationSize);
	outputAlloc = Allocation::createSized(rs, outElement, outAllocationSize);
	pixelAlloc = Allocation::createSized(rs, pixelElement, outAllocationSize);

	

	script = new ScriptC_convert(rs);

	script->set_inAllocation(inputAlloc);
	script->set_outAllocation(outputAlloc);

}

void FrameRenderer::renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer) {

	ANativeWindow* rWindow = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(rWindow, devSettings->frameWidth, outputFrameHeight, framePixelFormat);

	// Call the function pointer, which is set in the constructor
	(this->*processFrame)(inBuffer, rWindow);

	ANativeWindow_release(rWindow);
}


void FrameRenderer::processYUYV_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(firstFrameElementIndex);  // Bob Even first

	(script->*executeKernel)(pixelAlloc);  //TODO:  NEED TO CHECK AND SEE IF THIS WORKS!!!
	//script->forEach_convertFieldFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

	script->set_firstElement(secondFrameElementIndex);
	script->forEach_convertFieldFromYUYV(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}



void FrameRenderer::processUYVY_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(firstFrameElementIndex);  // Bob Even first

	if (interleaved)
		script->forEach_convertInterlacedFromUYVY(pixelAlloc);
	else
		{}// call renderscript for sequential conversion

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

	script->set_firstElement(secondFrameElementIndex);

	if (interleaved)
		script->forEach_convertFieldFromUYVY(pixelAlloc);
	else
        {}// call renderscript for sequential conversion

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}

void FrameRenderer::processRGB_NONE(CaptureBuffer* inBuffer, ANativeWindow* window) {


	// Write buffer directly to window, no conversion is necessary
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		memcpy(wBuffer.bits, inBuffer->start,  inBuffer->length);
		ANativeWindow_unlockAndPost(window);
	}
}