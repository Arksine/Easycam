//
// Created by Eric on 12/31/2015.
//

#include "FrameRenderer.h"
#include "util.h"

/*
The macros below reduce the amount of code necessary tremendously.  There are many
potential render paths, and a large nested switch statement is required to choose the
correct one.
TODO: 3/22/2016 - Add more pertinent information as to how the each macro expands
*/

/** TODO: 3/25/2016
Need to add more logging to see exactly how the variables are set.  Something is causing a SIGSEGV,
I have a feeling its something wrong with the way the pixel allocation is being processed.  I have
removed the function pointer to the kernel, perhaps that was the issue.  Will test and see
*/

#define INITRS(rsType, ppe, yuvType)													\
	initRenderscript(inputFrameWidth, inputFrameHeight, interleaved, ppe, yuvType);		\
	processFrame = &FrameRenderer::process ## rsType;									\
	LOGI("Render Method set as process %s", #rsType);									

#define COLORSWITCH(method)													                    \
	switch(devSettings->pixelFormat){											                \
		case V4L2_PIX_FMT_YUYV:													                \
			INITRS(YUYV_ ## method, 2, RS_YUV_NONE)												\
			LOGI("Pixel Format set as YUYV");                                                   \
			break;																                \
		case V4L2_PIX_FMT_UYVY:													                \
			INITRS(UYVY_ ## method, 2, RS_YUV_NONE)												\
			LOGI("Pixel Format set as UYVY");                                                   \
			break;																                \
		case V4L2_PIX_FMT_NV21:													                \
		    INITRS(Intrinsic_ ## method, 1, RS_YUV_NV21)										\
		    LOGI("Pixel Format set as NV21");                                                   \
			break;																                \
		case V4L2_PIX_FMT_YVU420:												                \
		    INITRS(Intrinsic_ ## method, 1, RS_YUV_YV12)										\
		    LOGI("Pixel Format set as YV12");                                                   \
			break;																                \
		case V4L2_PIX_FMT_RGB565:												                \
			INITRS(RGB_ ## method, 1, RS_YUV_NONE)												\
			LOGI("Pixel Format set as RGB565");                                                 \
			break;																                \
		default:																                \
			INITRS(YUYV_ ## method, 2, RS_YUV_NONE)												\
			LOGI("Pixel Format set as YUYV");                                                   \
	}                                                                                           \
	LOGI("Deinterlace Method set as %s", #method);

FrameRenderer::FrameRenderer(JNIEnv* jenv, jstring rsPath, DeviceSettings* devSettings) {

	int inputFrameWidth = devSettings->frameWidth;		// width of the incoming frame in pixels
	int inputFrameHeight;								// height of the incoming frame in pixels (this depe

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
	outputWindowWidth = devSettings->frameWidth;

    /*  TODO: 3/22/2016
    When recieving frames in V4L2_FIELD_TOP, V4L2_FIELD_BOTTOM, or V4L2_FIELD_ALTERNATE, does the
    driver report the full frame height, or half?  Need to make sure of this and correct the switch
    statement below if necessary.
    */

	switch(devSettings->field) {
		case V4L2_FIELD_NONE:	// Progressive Scan, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			outputWindowHeight = devSettings->frameHeight;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;   // not used
			COLORSWITCH(SCAN)
			break;
		case V4L2_FIELD_TOP:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputWindowHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			COLORSWITCH(SCAN)
			break;
		case V4L2_FIELD_BOTTOM:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputWindowHeight = devSettings->frameHeight / 2;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			COLORSWITCH(SCAN)
			break;
		case V4L2_FIELD_INTERLACED:
			interleaved = true;
			inputFrameHeight = devSettings->frameHeight;
			switch (devSettings->deintMethod) {
				case NONE:
					outputWindowHeight = devSettings->frameHeight;
					firstFrameElementIndex = 0;
					secondFrameElementIndex = 0;
					COLORSWITCH(SCAN)
					break;
				case DISCARD:
					outputWindowHeight = devSettings->frameHeight / 2;
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
					COLORSWITCH(DISCARD)
					break;
				case BOB:
					outputWindowHeight = devSettings->frameHeight / 2;
					// NTSC - Bottom field first
					if (devSettings->videoStandard == V4L2_STD_NTSC) {	
						firstFrameElementIndex = devSettings->frameWidth / 2;
						secondFrameElementIndex = 0;
					}
					//  PAL - Top Field first
					else {
						firstFrameElementIndex = 0;
						secondFrameElementIndex = devSettings->frameWidth / 2;
					}
					COLORSWITCH(BOB)
					break;
				//case BLEND:
					//outputWindowHeight = devSettings->frameHeight;
					//COLORSWITCH(BLEND, Blend?)
					//break;
				default:
					break;			
			}
			break;
		case V4L2_FIELD_SEQ_TB:
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = (devSettings->frameWidth*devSettings->frameHeight) / 4;
			switch (devSettings->deintMethod) {
                case NONE:
                    outputWindowHeight = devSettings->frameHeight;
                    COLORSWITCH(SCAN)
                    break;
                case DISCARD:
                    outputWindowHeight = devSettings->frameHeight / 2;
                    COLORSWITCH(DISCARD)
                    break;
                case BOB:
                    outputWindowHeight = devSettings->frameHeight / 2;
                    COLORSWITCH(BOB)
                    break;
                //case BLEND:
                    //outputWindowHeight = devSettings->frameHeight;
                    //COLORSWITCH(BLEND, Blend?)
                    //break;
                default:
                    break;
			}
			break;
		case V4L2_FIELD_SEQ_BT:
			interleaved = false;
			inputFrameHeight = devSettings->frameHeight;
			firstFrameElementIndex = (devSettings->frameWidth*devSettings->frameHeight) / 4;
            secondFrameElementIndex = 0;
			switch (devSettings->deintMethod) {
                case NONE:
                    outputWindowHeight = devSettings->frameHeight;
                    firstFrameElementIndex = 0;
                    COLORSWITCH(SCAN)
                    break;
                case DISCARD:
                    outputWindowHeight = devSettings->frameHeight / 2;
                    COLORSWITCH(DISCARD)
                    break;
                case BOB:
                    outputWindowHeight = devSettings->frameHeight / 2;
                    COLORSWITCH(BOB)
                    break;
                //case BLEND:
                    //outputFrameHeight = devSettings->frameHeight;
                    //COLORSWITCH(BLEND, Blend?)
                    //break;
                default:
                    break;
            }
			break;
		case V4L2_FIELD_ALTERNATE:	// Only half frame recd, no deinterlacing allowed
			interleaved = false;
			firstFrameElementIndex = 0;
			secondFrameElementIndex = 0;
			inputFrameHeight = devSettings->frameHeight / 2;
			outputWindowHeight = devSettings->frameHeight / 2;
			COLORSWITCH(SCAN)
			break;
		default:
			break;
	}

	LOGI("InputFrameHeight: %i", inputFrameHeight);
	LOGI("OutputWindowwidth: %i", outputWindowWidth);
	LOGI("OutputWindowHeight: %i", outputWindowHeight);
	LOGI("firstFrameElementIndex: %i", firstFrameElementIndex);
	LOGI("secondFrameElementIndex: %i", secondFrameElementIndex);

}

FrameRenderer::~FrameRenderer() {

}

void FrameRenderer::initRenderscript(int inputFrameWidth, int inputFrameHeight, bool interleaved, int pixelsPerElement, 
	RSYuvFormat yuvFmt = RS_YUV_NONE) {

	bool packedYUV;
	int inAllocWidth;
	int scriptOutAllocWidth;
	int intrinsOutAllocationSize;
	int inAllocationSize;
	int scriptOutAllocationSize;

	// Determine is the incoming format is packed YUV (YV12 or NV12)
	packedYUV = (yuvFmt != RS_YUV_NONE);

	// There are 32 bits per element for the input allocation, 
	// but only 16 bits per pixel, so we divide framewidth by 2
	// TODO: If we allow  32bpp input formats, such as ARGB32, this needs to be updated
	inAllocWidth = inputFrameWidth / 2;  	

	// If the color space is RGB565 we can output the the frame without conversion.
	if (pixelsPerElement == 1 && !packedYUV) { 

		// RGB565 output is only 16 bits per pixel, each element contains two pixels
		scriptOutAllocWidth = inputFrameWidth / 2;     
	}
	else {			

		// output is RGBA8888, number of elements = number of pixels
		scriptOutAllocWidth = inputFrameWidth;  
	}

	if (packedYUV) {

		// The intrinsic outputs RGBA, so its one elemenet per pixel
		intrinsOutAllocationSize = scriptOutAllocWidth * inputFrameHeight;
		
		// This isn't used as the allocation size is set through the typebuilder, but we will set it anyway
		inAllocationSize = inputFrameWidth * inputFrameHeight;
	}
	else {
		intrinsOutAllocationSize = 0;		// there is no intrinsic allocation
		inAllocationSize = inAllocWidth * inputFrameHeight;
	}

	scriptOutAllocationSize = scriptOutAllocWidth * outputWindowHeight;
	
	setupPixelAlloc(scriptOutAllocWidth, outputWindowHeight, pixelsPerElement, interleaved);
	
	script = new ScriptC_convert(rs);
	sp<const Element> outElement = Element::RGBA_8888(rs);

	if (packedYUV) {    // packedYUV types use ScriptIntrinsicYuvToRGB
		sp<const Element> inElement = Element::createPixel(rs, RS_TYPE_UNSIGNED_8, RS_KIND_PIXEL_YUV);
		Type::Builder yuvBuilder = Type::Builder(rs, inElement);
		yuvBuilder.setX(inputFrameWidth);
		yuvBuilder.setY(inputFrameHeight);
		yuvBuilder.setYuvFormat(yuvFmt);
		sp<const Type> yuvType = yuvBuilder.create();

		inputAlloc = Allocation::createTyped(rs, yuvType);
		intrinsOutAlloc = Allocation::createSized(rs, outElement, intrinsOutAllocationSize);
		intrinsic = ScriptIntrinsicYuvToRGB::create(rs, outElement);
		intrinsic->setInput(inputAlloc);

		// This is required to strip the field on interleaved frames when we discard or bob deinterlace
		script->set_inAllocation(intrinsOutAlloc);  
		                                          
	}
	else {
		sp<const Element> inElement = Element::U8_4(rs);
		inputAlloc = Allocation::createSized(rs, inElement, inAllocationSize);
		script->set_inAllocation(inputAlloc);
		
	}

	scriptOutputAlloc = Allocation::createSized(rs, outElement, scriptOutAllocationSize);
	script->set_outAllocation(scriptOutputAlloc);

}

void FrameRenderer::setupPixelAlloc(int pixelBufWidth, int pixelBufHeight, int pixelsPerElement, bool interleaved) {

	/*
	 The array pixelBuf is a backing store containing x values corresponding to the index
	 of each pixel in the input allocation.  This makes it easier to process YUV pixels that
	 have two pixels per element in the input allocation, and it also allows for more parallelism
	 as the kernel is split into each pixel instead of two pixels.
	 */
	int xVal = 0;
	int pixelBufAllocationSize = pixelBufWidth * pixelBufHeight;
	int32_t* pixelBuf = new int32_t[pixelBufAllocationSize];

	// If the input is interleaved, we only want a backing store for every other line.
	if (interleaved) {

		for (int y=0; y < pixelBufHeight; y++) {

			for (int x=0; x < pixelBufWidth; x++) {

				xVal = (x + ((2*y)*pixelBufWidth))/pixelsPerElement;
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
	pixelAlloc = Allocation::createSized(rs, pixelElement, pixelBufAllocationSize);
	pixelAlloc->copy1DFrom((void*)pixelBuf);
	delete [] pixelBuf;

}

void FrameRenderer::renderFrame(JNIEnv* jenv, jobject surface, CaptureBuffer* inBuffer) {

	ANativeWindow* rWindow = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(rWindow, outputWindowWidth, outputWindowHeight, frameWindowFormat);

	// Call the function pointer, which is set in the constructor
	(this->*processFrame)(inBuffer, rWindow);

	ANativeWindow_release(rWindow);
}

//----------------------------YUYV RENDER FUNCTIONS-----------------------------------

void FrameRenderer::processYUYV_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_offset(firstFrameElementIndex);
	script->forEach_convertFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

}

void FrameRenderer::processYUYV_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {
    // processRS_SCAN contains the necessary functionality to discard, so call that
    processYUYV_SCAN(inBuffer, window);
}

void FrameRenderer::processYUYV_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_offset(firstFrameElementIndex);
	script->forEach_convertFromYUYV(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

    CLEAR(wBuffer);
	script->set_offset(secondFrameElementIndex);
	script->forEach_convertFromYUYV(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}

//----------------------------UYVY RENDER FUNCTIONS-----------------------------------

void FrameRenderer::processUYVY_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_offset(firstFrameElementIndex);
	script->forEach_convertFromUYVY(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

}

void FrameRenderer::processUYVY_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {
    // processRS_SCAN contains the necessary functionality to discard, so call that
    processUYVY_SCAN(inBuffer, window);
}

void FrameRenderer::processUYVY_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

	inputAlloc->copy1DFrom(inBuffer->start);

	script->set_offset(firstFrameElementIndex);
	script->forEach_convertFromUYVY(pixelAlloc);

	// Write output buffers to the window, we are attempting bob deinterlacing, even first
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}

    CLEAR(wBuffer);
	script->set_offset(secondFrameElementIndex);
	script->forEach_convertFromUYVY(pixelAlloc);

	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		scriptOutputAlloc->copy1DTo(wBuffer.bits);
		ANativeWindow_unlockAndPost(window);
	}
}

//----------------------------RGB RENDER FUNCTIONS-----------------------------------

void FrameRenderer::processRGB_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

	// Write buffer directly to window, no conversion is necessary
	ANativeWindow_Buffer wBuffer;
	if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
		memcpy(wBuffer.bits, inBuffer->start,  inBuffer->length);
		ANativeWindow_unlockAndPost(window);
	}
}

// TODO:  3/21/2016 - Implement processRGB_BOB and processRGB_DISCARD

void FrameRenderer::processRGB_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

}

void FrameRenderer::processRGB_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {

}

//----------------------------INTRINSIC RENDER FUNCTIONS-----------------------------------

void FrameRenderer::processIntrinsic_SCAN(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsOutAlloc);

    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        intrinsOutAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

void FrameRenderer::processIntrinsic_DISCARD(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsOutAlloc);

    // Strip the first frame (even or odd depending of the frame element index)
    script->set_offset(firstFrameElementIndex);
    script->forEach_stripField(pixelAlloc);


    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        scriptOutputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

void FrameRenderer::processIntrinsic_BOB(CaptureBuffer* inBuffer, ANativeWindow* window) {

    inputAlloc->copy1DFrom(inBuffer->start);
    intrinsic->forEach(intrinsOutAlloc);

    // Strip the first frame (even or odd depending of the frame element index)
    script->set_offset(firstFrameElementIndex);
    script->forEach_stripField(pixelAlloc);

    // Write output buffers to the window, we are attempting bob deinterlacing, even first
    ANativeWindow_Buffer wBuffer;
    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        scriptOutputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

    CLEAR(wBuffer);
    script->set_offset(secondFrameElementIndex);
    script->forEach_stripField(pixelAlloc);

    if (ANativeWindow_lock(window, &wBuffer, NULL) == 0) {
        scriptOutputAlloc->copy1DTo(wBuffer.bits);
        ANativeWindow_unlockAndPost(window);
    }

}

