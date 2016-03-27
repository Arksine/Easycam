
#pragma version(1)
#pragma rs java_package_name(com.arksine.easycam)
#pragma rs_fp_relaxed

rs_allocation inAllocation;
rs_allocation outAllocation;

int32_t offset;         //  Offset to first Element to be processed. For interleaved frames the first element 0 for
						//  Odd frames, (frameWidth / 2) for even frames in YUY2 or other 16-bit
						//  formats, and frameWidth for RGBA and other 32-bit formats.  For sequential frames
						//  0 is the first element of the first half, and ((framewidth / 2)* frameheight) / 2,
						//  which is halfway though the input allocation

// Converts YUYV pixels from the input allocation into RGBA pixels.  This kernel is called on a
// buffer that contains x-index values for the input allocation.  This allows this kernel to
// convert entire frames, interleaved fields, and sequential fields.
void __attribute__((kernel)) convertFromYUYV(int32_t xIn, uint32_t x)
{
    uchar4 inElement;
    uchar4 outElement;
    uchar yValue;

	// The first element is an offset we add to the index received from the pixel alloc
    int32_t inputIndex = xIn + offset;

    inElement = rsGetElementAt_uchar4(inAllocation, inputIndex);

    // Bitwise AND with 0x0001 tells us if the current index of the pixelBuf is even or odd (zero indexed)
    // If its even, the element we need from the input allocation is x, otherwise its y.
	if ((x & (int32_t)0x0001) == 0)
	{
		//First pixel in a pair of YUV pixels
		yValue = inElement.x;
	}
	else
	{
		yValue = inElement.z;
	}

	outElement = rsYuvToRGBA_uchar4(yValue, inElement.y, inElement.w);

	rsSetElementAt_uchar4(outAllocation, outElement, x);

}

// Same as above, but converts from UYVY
void __attribute__((kernel)) convertFromUYVY(int32_t xIn, uint32_t x)
{
    uchar4 inElement;
    uchar4 outElement;
    uchar yValue;

    int32_t inputIndex = xIn + offset;

    inElement = rsGetElementAt_uchar4(inAllocation, inputIndex);

    if ((x & (int32_t)0x0001) == 0)
    {
        //First pixel in a pair of YUV pixels
        yValue = inElement.y;
    }
    else
    {
        yValue = inElement.w;
    }

    outElement = rsYuvToRGBA_uchar4(yValue, inElement.x, inElement.z);

   	rsSetElementAt_uchar4(outAllocation, outElement, x);

}

// Uses the x-index values from the pixel buffer to strip fields from the input allocation.
// No other processing is done.
void __attribute__((kernel)) stripField(int32_t xIn, uint32_t x ) {

	uchar4 element;

    int32_t inputIndex = xIn + offset;

    element = rsGetElementAt_uchar4(inAllocation, inputIndex);

    rsSetElementAt_uchar4(outAllocation, element, x);
}

