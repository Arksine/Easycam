/*
 * VideoDevice.h
 *
 *  Created on: Nov 4, 2014
 *      Author: Eric
 */

#ifndef VIDEODEVICE_H_
#define VIDEODEVICE_H_

#include "util.h"

/* Class: Video Device
 * Encapsulates a V4l2 device, exposing all functionality
 * related to opening, streaming, and closing a device.
 */
class VideoDevice {
public:
	VideoDevice(DeviceSettings* dSets);
	virtual ~VideoDevice();

	/* The below functions control a capture device.  A note about how they work:
	 * A device must be opened, initialzed, and set to streamon (start_capture)
	 * before you can capture frames.  You can "pause" the stream with
	 * stop_cature.  stop_device deletes buffers and closes the device completely.
	 * If you stop a device it must be opened, initialized, and the stream turned
	 * on before you can run process capture again.
	 */
	int openDevice();   // attempt to open device_name and get file descriptor
	int initDevice();   // initialize buffers
	int startCapture(); // enqueue buffers and turn stream on
	int stopCapture();  // turn stream off
	CaptureBuffer * processCapture();   // dequeue a frame and bring it to user space
	void stopDevice();  // shut down device

	// static function to detect a device
	static char* detectDevice(const char* devLocation);

	bool videoDeviceAttached() {
		if (fileDescriptor == -1)
			return false;
		else
			return true;
	}

private:
	unsigned int bufferCount;  // actual number of buffers allocated
	CaptureBuffer* frameBuffers;
	int fileDescriptor;
	DeviceSettings* devSettings;

	int curBufferIndex;  // the index for the last buffer read into memory

	int uninitDevice();
	int closeDevice();
	int initMemmap();
	int readFrame();

	//v4l2 helper functions
	static int v4l2_open(const char* devLocation);
	static int v4l2_close(int fd);

};

#endif /* VIDEODEVICE_H_ */
