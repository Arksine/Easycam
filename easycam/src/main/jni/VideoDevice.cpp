/*
 * VideoDevice.cpp
 *
 *  Created on: Nov 4, 2014
 *      Author: Eric
 */

#include "VideoDevice.h"
#include "util.h"
#include <sys/mman.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <malloc.h>
#include <cassert>

/**
 * TODO: Need to go through all functions and make sure that the functionality is correct now that
 *       I am creating device settings directly, rather than passing enums and determining settings here
 */

VideoDevice::VideoDevice (DeviceSettings* dSets) {

	devSettings = dSets;
	bufferCount = 0;
	frameBuffers = NULL;
	fileDescriptor = -1;
    curBufferIndex = 0;

}

VideoDevice::~VideoDevice() {
	stopDevice();
}

/* Open the video device at the named device node.
 *
 *
 * Returns SUCCESS_LOCAL if the device was found and opened and ERROR_LOCAL if
 * an error occurred.
 */
int VideoDevice::openDevice() {

	fileDescriptor = v4l2_open(devSettings->location);

	if (fileDescriptor == -1)
		return ERROR_LOCAL;

    return SUCCESS_LOCAL;
}

/* Initialize video device with the given frame size.
 *
 * Initializes the device as a video capture device (must support V4L2) and
 * checks to make sure it has the streaming I/O interface. Configures the device
 * to crop the image to the given dimensions and initailizes a memory mapped
 * frame buffer.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::initDevice() {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    v4l2_std_id stdId;
    unsigned int min;

    CLEAR(cap);
    if(-1 == xioctl(fileDescriptor, VIDIOC_QUERYCAP, &cap)) {
        if(EINVAL == errno) {

            return ERROR_LOCAL;
        } else {
            return errnoexit("VIDIOC_QUERYCAP");
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("device is not a video capture device");
        return ERROR_LOCAL;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("device does not support streaming i/o");
        return ERROR_LOCAL;
    }


	// Set the input if required
	if (devSettings->input > -1)
	{
		int input = devSettings->input;
		if (-1 == xioctl (fileDescriptor, VIDIOC_S_INPUT, &input)) {
			return errnoexit("VIDIOC_S_INPUT");
		}
	}

    CLEAR(stdId);
	stdId = devSettings->videoStandard;

	if(-1 == xioctl(fileDescriptor, VIDIOC_S_STD, &stdId)) {
		   return errnoexit("VIDIOC_S_STD");
	   }

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(0 == xioctl(fileDescriptor, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if(-1 == xioctl(fileDescriptor, VIDIOC_S_CROP, &crop)) {
            switch(errno) {
                case EINVAL:
                    break;
                default:
                    break;
            }
        }
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt.fmt.pix.width = devSettings->frameWidth;
    fmt.fmt.pix.height = devSettings->frameHeight;
	fmt.fmt.pix.pixelformat = devSettings->pixelFormat;
    fmt.fmt.pix.field = devSettings->field;

    if(-1 == xioctl(fileDescriptor, VIDIOC_S_FMT, &fmt)) {
        return errnoexit("VIDIOC_S_FMT");
    }

	// TODO:  We should compare the values returned from IO control to the values we passed to it.
	//        If they were not accepted and default values were sent back we need to know about it
	//        so rendering functionality isn't broken.


    return initMemmap();
}

/* Initialize memory mapped buffers for video frames.
 *
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::initMemmap() {
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = devSettings->numBuffers;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if(-1 == xioctl(fileDescriptor, VIDIOC_REQBUFS, &req)) {
		if(EINVAL == errno) {
			LOGE("device does not support memory mapping");
			return ERROR_LOCAL;
		} else {
			return errnoexit("VIDIOC_REQBUFS");
		}
	}

	if(req.count < 2) {
		LOGE("Insufficient buffer memory");
		return ERROR_LOCAL;
	}

	frameBuffers = (CaptureBuffer *)calloc(req.count, sizeof(*frameBuffers));
	if(!frameBuffers) {
		LOGE("Out of memory");
		return ERROR_LOCAL;
	}

	for(bufferCount = 0; bufferCount < req.count; ++bufferCount) {
		struct v4l2_buffer buf;
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = bufferCount;

		if(-1 == xioctl(fileDescriptor, VIDIOC_QUERYBUF, &buf)) {
			return errnoexit("VIDIOC_QUERYBUF");
		}

		frameBuffers[bufferCount].length = buf.length;
		frameBuffers[bufferCount].start = mmap(NULL, buf.length,
		                                         PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, buf.m.offset);

		if(MAP_FAILED == frameBuffers[bufferCount].start) {
			return errnoexit("mmap");
		}
	}

	LOGI("Frame Buffer Length (bytes): %i", frameBuffers[0].length);

	return SUCCESS_LOCAL;
}

/* Begins capturing video frames from a previously initialized device.
 *
 * The buffers in frameBuffers are handed off to the device.
 * *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::startCapture() {
    unsigned int i;
    enum v4l2_buf_type type;

    for(i = 0; i < bufferCount; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(-1 == xioctl(fileDescriptor, VIDIOC_QBUF, &buf)) {
            return errnoexit("VIDIOC_QBUF");
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fileDescriptor, VIDIOC_STREAMON, &type)) {
        return errnoexit("VIDIOC_STREAMON");
    }

    return SUCCESS_LOCAL;
}

/* Request a frame of video from the device to be output into the rgb
 * and y buffers.
 *
 * If the descriptor is not valid, no frame will be read.
 *
 * Returns a pointer to the latest buffer read into memory from the device
 *
 */
CaptureBuffer * VideoDevice::processCapture() {

	if(fileDescriptor == -1) {
		return NULL;
	}

	for(;;) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fileDescriptor, &fds);

		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		int result = select(fileDescriptor + 1, &fds, NULL, NULL, &tv);
		if(-1 == result) {
			if(EINTR == errno) {
				continue;
			}
			errnoexit("select");
		} else if(0 == result) {
			LOGE("select timeout, likely can't process chosen TV standard");
			sleep(1);
			break;
		}

		if(readFrame() == 1) {
			return &frameBuffers[curBufferIndex];
		}
	}

	return NULL;
}

/* Read a single frame of video from the device into a buffer and sets the index of the buffer
 * read.
 *
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::readFrame() {
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(fileDescriptor, VIDIOC_DQBUF, &buf)) {
        switch(errno) {
            case EAGAIN:
                return 0;
            case EIO:
            default:
                return errnoexit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < bufferCount);


    // convert and copy the buffer for rendering
    curBufferIndex = (int)(buf.index);

    if(-1 == xioctl(fileDescriptor, VIDIOC_QBUF, &buf)) {
    	return errnoexit("VIDIOC_QBUF");
    }

    return 1;
}

/* Stop capturing, uninitialize the device and free all memory. */
void VideoDevice::stopDevice() {
	stopCapture();
	uninitDevice();
	closeDevice();

}

/* Unconfigure the video device for capturing.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::stopCapture() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 != fileDescriptor && -1 == xioctl(fileDescriptor, VIDIOC_STREAMOFF, &type)) {
        return errnoexit("VIDIOC_STREAMOFF");
    }

    return SUCCESS_LOCAL;
}

/* Unmap and free memory-mapped frame buffers from the device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::uninitDevice() {
	for (unsigned int i = 0; i < bufferCount; ++i) {
		if (-1 == munmap(frameBuffers[i].start, frameBuffers[i].length)) {
			return errnoexit("munmap");
		}
	}

	free(frameBuffers);
	return SUCCESS_LOCAL;
}


/* Close a file descriptor.
 *
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::closeDevice() {

	return v4l2_close(fileDescriptor);
}

// TODO: 3/22/2016 - need to return a string rather than a char*.  Also need to set that up
//                   in the device settings structure
/**
 *   Helper function to attempt to find a specific device
 */
char* VideoDevice::detectDevice(const char* devLocation) {
	struct v4l2_capability cap;
	int fd = -1;

	fd = v4l2_open(devLocation);
	if (fd == -1)
	{
		return strdup("NO_DEVICE");
	}

	// Get device capabilities
	CLEAR(cap);
	if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		return strdup("NO_DEVICE");
	}

	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		return strdup("NO_DEVICE");
	}

	if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
		return strdup("NO_DEVICE");
	}

	LOGI("Driver detected as: %s", cap.driver);

	// Close Device
	if(v4l2_close(fd) != SUCCESS_LOCAL) {
		return strdup("NO_DEVICE");
	}

    char* deviceName = strdup((const char*)cap.driver);
	return deviceName;
}


// Helper functions to open and close devices, to keep from repeating code
int VideoDevice::v4l2_open(const char* devLocation) {

	struct stat st;
	int fd = - 1;
	if(-1 == stat(devLocation, &st)) {
		LOGE("Cannot identify '%s': %d, %s", devLocation, errno, strerror(errno));
		return ERROR_LOCAL;
	}

	if(!S_ISCHR(st.st_mode)) {
		LOGE("%s is not a valid device", devLocation);
		return ERROR_LOCAL;
	}

	fd = open(devLocation, O_RDWR | O_NONBLOCK, 0);
	if(-1 == fd) {
		LOGE("Cannot open '%s': %d, %s", devLocation, errno, strerror(errno));
		if(EACCES == errno) {
			LOGE("Insufficient permissions on '%s': %d, %s", devLocation, errno,
					strerror(errno));
		}
		return ERROR_LOCAL;
	}

	return fd;
}

int VideoDevice::v4l2_close(int fd) {
	int result = SUCCESS_LOCAL;
	if(-1 != fd && -1 == close(fd)) {
		result = errnoexit("close");
	}
	fd = -1;
	return result;
}

