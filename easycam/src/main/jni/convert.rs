
#pragma version(1)
#pragma rs java_package_name(com.arksine.easycam)
#pragma rs_fp_relaxed

rs_allocation outputOdd;
rs_allocation outputEven;

// number of elements representing the width of a frame from the input buffer
// (frameWidth * 2 bytes per pixel / 4 bytes per element)
int32_t xElements;

void __attribute__((kernel)) convertFromYUYV(uchar4 in, uint32_t x)
{
    uchar4 first;
    uchar4 second;

	// get the y index by dividing the current position by the frameWidth.  All decimals should
	// get floored
	uint32_t yOutIndex = (x / xElements);

	// since we are splitting the input allocation into two outputs
	uint32_t xOffset = (yOutIndex + 1) / 2;
	xOffset = xOffset * xElements;

	// offset the x by subtracting the yIndex multiplied by the number of elements in the frame width.
    uint32_t xOutIndex = 2*(x - xOffset);

    first = rsYuvToRGBA_uchar4(in.x, in.y, in.w);
    second = rsYuvToRGBA_uchar4(in.z, in.y, in.w);

	// binary & the index to see if the data is part of an even or odd frame
    yOutIndex &= (uint32_t)0x0001;

	if (yOutIndex) {
		// This is the second field, so output to Even Allocation
        rsSetElementAt_uchar4(outputEven, first, xOutIndex);
        rsSetElementAt_uchar4(outputEven, second, xOutIndex+1);
    }
    else
    {
        rsSetElementAt_uchar4(outputOdd, first, xOutIndex);
        rsSetElementAt_uchar4(outputOdd, second, xOutIndex+1);
    }



}

void __attribute__((kernel)) convertFromUYVY(uchar4 in, uint32_t x)
{
    uchar4 first;
    uchar4 second;

    // get the y index by dividing the current position by the frameWidth.  All decimals should
    // get floored
    uint32_t yOutIndex = (x / xElements);

    // since we are splitting the input allocation into two outputs
    uint32_t xOffset = (yOutIndex + 1) / 2;
    xOffset = xOffset * xElements;

    // offset the x by subtracting the yIndex multiplied by the number of elements in the frame width.
    uint32_t xOutIndex = 2*(x - xOffset);

    first = rsYuvToRGBA_uchar4(in.y, in.x, in.z);
    second = rsYuvToRGBA_uchar4(in.w, in.x, in.z);

    // binary & the index to see if the data is part of an even or odd frame
    yOutIndex &= (uint32_t)0x0001;

    if (yOutIndex) {
        // This is the second field, so output to Even Allocation
        rsSetElementAt_uchar4(outputEven, first, xOutIndex);
        rsSetElementAt_uchar4(outputEven, second, xOutIndex+1);
    }
    else
    {
        rsSetElementAt_uchar4(outputOdd, first, xOutIndex);
        rsSetElementAt_uchar4(outputOdd, second, xOutIndex+1);
    }

}