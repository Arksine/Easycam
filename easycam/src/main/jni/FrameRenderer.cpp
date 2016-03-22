//
// Created by Eric on 12/31/2015.
//

#include "FrameRenderer.h"
#include "util.h"

#define INITRS(process, forEach, yuvType)			            \
	initRenderscript(inputFrameHeight, interleaved, yuvType);   \
	processFrame = &FrameRenderer:: ## process;		            \
	executeKernel = &ScriptC_convert:: ## forEach;

/*
*	deint - deinterlacing method.  IE: NONE, DISCARD, BOB, BLEND.
*	ft - field type.  Typically Frame, it will be Field if we are deinterlacing
*/
#define COLORSWITCH(method, ft)													                \
	switch(devSettings->pixelFormat){											                \
		case V4L2_PIX_FMT_YUYV:													                \
			INITRS(processRS_ ## method, forEach_convert ## ft ## FromYUYV, RS_YUV_NONE)		\
			break;																                \
		case V4L2_PIX_FMT_UYVY:													                \
			INITRS(processRS_ ## method, forEach_convert ## ft ## FromUYVY, RS_YUV_NONE)		\
			break;																                \
		case V4L2_PIX_FMT_NV21:													                \
		    INITRS(processIntrinsic_ ## method, forEach_stripField, RS_YUV_NV21)                \
			break;																                \
		case V4L2_PIX_FMT_YVU420:												                \
		    INITRS(processIntrinsic_ ## method, forEach_stripField, RS_YUV_YV12)                \
			break;																                \
		case V4L2_PIX_FMT_RGB565:												                \
			if (strncmp(#method, "NONE", 4) == 0) {								                \
				processFrame = &FrameRenderer::processRGB_SCAN;					                \
			}																	                \
			else {																                \
				INITRS(processRGB_ ## method, forEach_stripField ## ft ## FromRGB, RS_YUV_NONE)	\
			}																	                \
			break;																                \
		default:																                \
			INITRS(processRS_ ## method, forEach_convert ## ft ## FromYUYV, RS_YUV_NONE)		\
	}

FrameRenderer::FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceSettings* dSets) {

	devSettings = dSets;

	int inputFrameHeight;		// height of the incoming frame in pixels

	bool interleaved = false;   //
	bool packedYUV = false;
	RSYuvFormat yuvFmt;

	// intialize the renderscript reference
	const char* path = jenv->GetStringUTFChars(rsPath, NULL);
	rs = new RS();
	rs->init(path);
	jenv->ReleaseStringUTFChars(rsPath, path);

	// Set the frame output pixel format, which will always be RGBA8888 unless the
	// input colorspace is RGB565
	if (devSettings->pixelFormat == V4L2_PIX_FMT_RGB565) {
		frameWindowFormat = WINDOW_FORMAT_RGB_565;
	}
	else {
		frameWindowFormat = WINDOW_FORMAT_RGBA_8888;
	}

	switch(devSettings->field) {
		case V4L2_FIELD_NONE:	// Progressive Scan, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			outputFrameHeight = devSettings->frameHeight;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;   // not used
			COLORSWITCH(SCAN, Frame)
			break;
		case V4L2_FIELD_TOP:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputFrameHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			COLORSWITCH(SCAN, Frame)
			break;
		case V4L2_FIELD_BOTTOM:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputFrameHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			COLORSWITCH(SCAN, Frame)
			break;
		case V4L2_FIELD_INTERLACED:
			interleaved = true;
			inputFrameHeight = devSettings->frameHeight;
			switch (devSettings->deintMethod) {
				case NONE:
					outputFrameHeight = devSettings->frameHeight;
					firstFrameElementIndex = 0;
					secondFrameElementIndex = 0;
					COLORSWITCH(SCAN, Frame)
					break;
				case DISCARD:
					outputFrameHeight = devSettings->frameHeight / 2;
					// NTSC - Bottom field first
					if (devSettings->videoStandard == V4L2_STD_NTSC) {	
						firstFrameElementIndex = devSettings->frameWidth / 2;
						secondFrameElementIndex = 0;
					}
					// PAL - Top Field first
					else {
						firstFrameElementIndex = 0;
						secondFrameElementIndex = devSettings->frameWidth / 2;
					}
					COLORSWITCH(DISCARD, Field)
					break;
				case BOB:
					outputFrameHeight = devSettings->frameHeight / 2;
					// NTSC - Bottom field first
					if (devSettings->videoStandard == V4L2_STD_NTSC) {	
						firstFrameElementIndex = devSettings->frameWidth / 2;
						secondFrameElementIndex = 0;
					}
					// NTSC - Bottom field first
					else {
						firstFrameElementIndex = 0;
						secondFrameElementIndex = devSettings->frameWidth / 2;
					}
					COLORSWITCH(BOB, Field)
					break;
				//case BLEND:
					//outputFrameHeight = devSettings->frameHeight;
					//COLORSWITCH(BLEND, Blend?)
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
			COLORSWITCH(NONE, Frame)
			break;
		default:
			break;
	}

}

FrameRenderer::~FrameRenderer() {

}

void FrameRenderer::initRenderscript(int inputFrameHeight, bool interleaved, RSYuvFormat yuvFmt = RS_YUV_NONE) {

	// Determine is the incoming format is packed YUV (YV12 or NV12)
	bool packedYUV = (yuvFmt != RS_YUV_NONE);

	// There are 32 bits per element for the input allocation, 
	// but only 16 bits per pixel, so we divide framewidth by 2
	// TODO: If we allow other input formats, such as ARGB32, this needs to be updated
	int inAllocWidth = devSettings->frameWidth / 2;  	
	int outAllocWidth;
	if (devSettings->pixelFormat == V4L2_PIX_FMT_RGB565) {  
	
		// RGB565 output is only 16 bits per pixel
		outAllocWidth = devSettings->frameWidth / 2;     
	}
	else {			

		// output is RGBA8888, number of elements = number of pixels
		outAllocWidth = devSettings->frameWidth;  
	}
	 	
	int intrinsAllocationSize;
	int inAllocationSize;
	if (packedYUV) {
		// The intrinsic outputs RGBA, so its one elemenet per pixel
		intrinsAllocationSize = outAllocWidth * inputFrameHeight;
		
		// TODO:  For right now this isnt used
		inAllocationSize = devSettings->frameWidth * inputFrameHeight;
	}
	else {
		intrinsAllocationSize = 0;		// there is no intrinsic allocation
		inAllocationSize = inAllocWidth * inputFrameHeight;
	}
	int outAllocationSize = outAllocWidth * outputFrameHeight;
	

	/*
	 The array pixelBuf is a backing store containing x values corresponding to the index
	 of each pixel in the input allocation.  This makes it easier to process YUV pixels that
	 have two pixels per element in the input allocation, and it also allows for more parallelism
	 as the kernel is split into each pixel instead of two pixels.
	 */
	int xVal = 0;
	int32_t* pixelBuf = new int32_t[outAllocationSize];

	// The pixel buf is the same size as the output allocation (one element per pixel), but
	// the variables below allow for friendlier names when populating the pixelBuf
	int pixelBufWidth = outAllocWidth;
	int pixelBufHeight = outputFrameHeight;
	int pixelsPerElement;           // number of pixels stored in one element for the input allocation
	if (packedYUV || devSettings->pixelFormat == V4L2_PIX_FMT_RGB565)
		pixelsPerElement = 1;
	else
		pixelsPerElement = 2;

	// If the input is interleaved, we only want a backing store for every other line.
	if (interleaved) {

		for (int y=0; y < pixelBufHeight; y++) {

			for (int x=0; x < pixelBufWidth; x++) {

				xVal = (x + ((2*y)*pixelBufWidth)/pixelsPerElement;
				pixelBuf[x + (y*pixelBufWidth)] = xVal;
			}
		}
		
	}
	else {
		// Calculate the x values for the input allocation in sequential fields
		for (int y=0; y < pixelBufHeight; y++) {

			for (int x=0; x < pixelBufWidth; x++) {
								
				xVal = (x + (y*pixelBufWidth))/pixelsPerElement;
				pixelBuf[x + (y*pixelBufWidth)] = xVal;
			}
		}		
	}

	sp<const Element> pixelElement = Element::I32(rs);
	pixelAlloc = Allocation::createSized(rs, pixelElement, outAllocationSize);
	pixelAlloc->copy1DFrom((void*)pixelBuf);
	delete [] pixelBuf;

	script = new ScriptC_convert(rs);
	sp<const Element> outElement = Element::RGBA_8888(rs);

	if (packedYUV) {    // packedYUV types use ScriptIntrinsicYuvToRGB
		sp<const Element> inElement = Element::createPixel(rs, RS_TYPE_UNSIGNED_8, RS_KIND_PIXEL_YUV);
		sp<const Type::Builder> yuvBuilder = Type.Builder(rs, inElement);
		yuvBuilder.setX(devSettings->frameWidth);
		yuvBuilder.setY(inputFrameHeight);
		yuvBuilder.setYuvFormat(yuvFmt);
		sp<const Type> yuvType = yuvBuilder.create();

		inputAlloc = Allocation::createTyped(rs, yuvType);
		intrinsAlloc = Allocation::createSized(rs, outElement, intrinsAllocationSize);
		intrinsic = ScriptIntrinsicYuvToRGB::create(rs, outElement);
		intrinsic->setInput(inputAlloc);
		script->set_inAllocation(intrinsAlloc);  // This is required to strip the field on interleaved
		                                         // frames when we discard or bob deinterlace
	}
	else {
		sp<const Element> inElement = Element::U8_4(rs);
		inputAlloc = Allocation::createSized(rs, inElement, inAllocationSize);
		script->set_inAllocation(inputAlloc);
		
	}

	outputAlloc = Allocation::createSized(rs, outElement, outAllocationSize);
	script->set_outAllocation(outputAlloc);

}

void FrameRenderer::renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer) {

	ANativeWindow* rWindow = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(rWindow, devSettings->frameWidth, outputFrameHeight, frameWindowFormat);

	// Call the function pointer, which is set in the constructor
	(this->*processFrame)(inBuffer, rWindow);

	ANativeWindow_release(rWindow);
}

void FrameRenderer::processRS_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(firstFrameElementIndex);
	(script->*executeKernel)(pixelAlloc);  //TODO:  NEED TO CHECK AND SEE IF THIS WORKS!!!
	//script->forEach_convertFieldFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

}

void FrameRenderer::processRS_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {
    // processRS_SCAN contains the necessary functionality to discard, so call that
    processRS_SCAN(inBuffer, window);
}

void FrameRenderer::processRS_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_firstElement(firstFrameElementIndex);
	(script->*executeKernel)(pixelAlloc);  //TODO:  NEED TO CHECK AND SEE IF THIS WORKS!!!
	//script->forEach_convertFieldFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

    CLEAR(wBuffer);
	script->set_firstElement(secondFrameElementIndex);
	(script->*executeKernel)(pixelAlloc);
	// script->forEach_convertFieldFromYUYV(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		outputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}

void FrameRenderer::processRGB_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

	// Write buffer directly to window, no conversion is necessary
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		memcpy(wBuffer.bits, inBuffer->start,  inBuffer->length);
		ANativeWindow_unlockAndPost(window);
	}
}

// TODO:  3/21/2016 - Add functions to process RGB deinterlacing (BOB and DISCARD)

void FrameRenderer::processIntrinsic_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsAlloc);

    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        intrinsAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

void FrameRenderer::processIntrinsic_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsAlloc);

    // Strip the first frame (even or odd depending of the frame element index)
    script->set_firstElement(firstFrameElementIndex);
    (script->*executeKernel)(pixelAlloc);

    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        outputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

void FrameRenderer::processIntrinsic_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsAlloc);

    // Strip the first frame (even or odd depending of the frame element index)
    script->set_firstElement(firstFrameElementIndex);
    (script->*executeKernel)(pixelAlloc);

    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        outputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

    CLEAR(wBuffer);
    script->set_firstElement(secondFrameElementIndex);
    (script->*executeKernel)(pixelAlloc);
    // script->forEach_convertFieldFromYUYV(pixelAlloc);

    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        outputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

