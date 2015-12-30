/*
 * VideoDevice.cpp
 *
 *  Created on: Nov 4, 2014
 *      Author: Eric
 */

#include "VideoDevice.h"
#include "util.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <malloc.h>
#include <cassert>

/* TODO: Use renderscript to do color conversion in place of libyuv.
 * The best way to accomplish this is likely to copy the v4l frame buffer into an Allocation,
 * and pass that allocation back to Java.  The other option is to do the complete conversion
 * in C++/JNI, and pass the result back to Java.*/

// Prototypes for helper functions
int v4l2_open(const char* dev_name);
int v4l2_close(int fd);

VideoDevice::VideoDevice (DeviceSettings devSets) {

	device_sets = devSets;

	// assign V4L2/LIBYUV specific settings
	switch(devSets.device_type) {
	case UTV007:
		device_sets.pixel_format =  V4L2_PIX_FMT_YUYV;
		break;
	case EMPIA:
		device_sets.pixel_format =  V4L2_PIX_FMT_YUYV;
		break;
	case STK1160:
		device_sets.pixel_format =  V4L2_PIX_FMT_UYVY;
		break;
	case SOMAGIC:
		device_sets.pixel_format =  V4L2_PIX_FMT_UYVY;
		break;
	default:
		device_sets.pixel_format =  V4L2_PIX_FMT_YUYV;
	}

	buffer_count = 0;
	frame_buffers = NULL;


	file_descriptor = -1;

    curBufferIndex = 0;

	int area = devSets.frame_width * devSets.frame_height;


}

VideoDevice::~VideoDevice() {
	stop_device();
}

/* Open the video device at the named device node.
 *
 * dev_name - the path to a device, e.g. /dev/video0
 * fd - an output parameter to store the file descriptor once opened.
 *
 * Returns SUCCESS_LOCAL if the device was found and opened and ERROR_LOCAL if
 * an error occurred.
 */
int VideoDevice::open_device() {

	file_descriptor = v4l2_open(device_sets.device_name);

	if (file_descriptor == -1)
		return ERROR_LOCAL;

    return SUCCESS_LOCAL;
}

/* Initialize memory mapped buffers for video frames.
 *
 * fd - a valid file descriptor pointing to the camera device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::init_mmap() {
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = device_sets.num_buffers;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(file_descriptor, VIDIOC_REQBUFS, &req)) {
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

    frame_buffers = (buffer*)calloc(req.count, sizeof(*frame_buffers));
    if(!frame_buffers) {
        LOGE("Out of memory");
        return ERROR_LOCAL;
    }

    for(buffer_count = 0; buffer_count < req.count; ++buffer_count) {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = buffer_count;

        if(-1 == xioctl(file_descriptor, VIDIOC_QUERYBUF, &buf)) {
            return errnoexit("VIDIOC_QUERYBUF");
        }

        frame_buffers[buffer_count].length = buf.length;
        frame_buffers[buffer_count].start = mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, buf.m.offset);

        if(MAP_FAILED == frame_buffers[buffer_count].start) {
            return errnoexit("mmap");
        }
    }

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
int VideoDevice::init_device() {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    v4l2_std_id std_id;
    unsigned int min;

    CLEAR(cap);
    if(-1 == xioctl(file_descriptor, VIDIOC_QUERYCAP, &cap)) {
        if(EINVAL == errno) {
            LOGE("not a valid V4L2 device");
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


 /*  Input selection isn't helping EMPIA or SOMAGIC.  Commenting out until further research is done
    // Need to set empia devices to composite input
    if(device_sets.device_type == EMPIA) {
        int input = 2;

        if (-1 == xioctl (file_descriptor, VIDIOC_S_INPUT, &input)) {
                return errnoexit("VIDIOC_S_INPUT");
            }
    }

    to be sure none of this is needed.
    Commented Somagic selection out for the time being.  Didn't seem to help much.


     * Info:
     * Somagic based devices do NOT like to start streaming before input is received
     * Iterate through

    if(device_sets.device_type == SOMAGIC)
    {
    	struct timeval start, end, waitTime;
    	waitTime.tv_sec = 10;
    	waitTime.tv_usec = 0;

    	struct v4l2_input inputInfo;
    	bool signalDetected = false;
    	int input;


    	 * Iterate through the available inputs for 10 seconds, looking for a signal.
    	 * If none is found we will exit device initialization and return an error

    	gettimeofday(&start, NULL);
    	while (!signalDetected && (end.tv_sec < (start.tv_sec + waitTime.tv_sec))) {
			input = 0;
			CLEAR(inputInfo);
			while (-1 != xioctl (file_descriptor, VIDIOC_ENUMINPUT, &inputInfo))
			{

				if (!(inputInfo.status & V4L2_IN_ST_NO_SIGNAL)) {

					if (-1 == xioctl (file_descriptor, VIDIOC_S_INPUT, &input)) {
						return errnoexit("VIDIOC_S_INPUT");
					}

					signalDetected = true;
					break;
				}

				CLEAR(inputInfo);
				input++;
			}
			sleep(1);
			gettimeofday(&end, NULL);
    	}

    	if (!signalDetected) {

    		return errnoexit("Video Source Not Found: SOMAGIC");
    	}
    }*/


    CLEAR(std_id);
	switch(device_sets.standard_id) {
	case NTSC:
		std_id = V4L2_STD_NTSC;
		break;
	case PAL:
		std_id = V4L2_STD_PAL;
		break;
	default:
		std_id = V4L2_STD_NTSC;
	}

	if(-1 == xioctl(file_descriptor, VIDIOC_S_STD, &std_id)) {
		   return errnoexit("VIDIOC_S_STD");
	   }

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(0 == xioctl(file_descriptor, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if(-1 == xioctl(file_descriptor, VIDIOC_S_CROP, &crop)) {
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

    fmt.fmt.pix.width = device_sets.frame_width;
    fmt.fmt.pix.height = device_sets.frame_height;

    fmt.fmt.pix.pixelformat = device_sets.pixel_format;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if(-1 == xioctl(file_descriptor, VIDIOC_S_FMT, &fmt)) {
        return errnoexit("VIDIOC_S_FMT");
    }


    return init_mmap();
}

/* Unmap and free memory-mapped frame buffers from the device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::uninit_device() {
    for(unsigned int i = 0; i < buffer_count; ++i) {
        if(-1 == munmap(frame_buffers[i].start, frame_buffers[i].length)) {
            return errnoexit("munmap");
        }
    }

    free(frame_buffers);
    return SUCCESS_LOCAL;
}

/* Close a file descriptor.
 *
 * fd - a pointer to the descriptor to close, which will be set to -1 on success
 *      or fail.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::close_device() {

    return v4l2_close(file_descriptor);
}

/* Begins capturing video frames from a previously initialized device.
 *
 * The buffers in FRAME_BUFFERS are handed off to the device.
 *
 * fd - a valid file descriptor to the device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::start_capture() {
    unsigned int i;
    enum v4l2_buf_type type;

    for(i = 0; i < buffer_count; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(-1 == xioctl(file_descriptor, VIDIOC_QBUF, &buf)) {
            return errnoexit("VIDIOC_QBUF");
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(file_descriptor, VIDIOC_STREAMON, &type)) {
        return errnoexit("VIDIOC_STREAMON");
    }

    return SUCCESS_LOCAL;
}

/* Read a single frame of video from the device into a buffer.
 *
 * The resulting image is stored in RGBA format across two buffers, rgb_buffer
 * and y_buffer.
 *
 * fd - a valid file descriptor pointing to the camera device.
 * frame_buffers - memory mapped buffers that contain the image from the device.
 * width - the width of the image.
 * height - the height of the image.
 * rgb_buffer - output buffer for RGB data.
 * y_buffer - output buffer for alpha (Y) data.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::read_frame() {
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(file_descriptor, VIDIOC_DQBUF, &buf)) {
        switch(errno) {
            case EAGAIN:
                return 0;
            case EIO:
            default:
                return errnoexit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < buffer_count);


    // convert and copy the buffer for rendering
    curBufferIndex = (int)(buf.index);

    if(-1 == xioctl(file_descriptor, VIDIOC_QBUF, &buf)) {
    	return errnoexit("VIDIOC_QBUF");
    }

    return 1;
}


/* Unconfigure the video device for capturing.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int VideoDevice::stop_capture() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 != file_descriptor && -1 == xioctl(file_descriptor, VIDIOC_STREAMOFF, &type)) {
        return errnoexit("VIDIOC_STREAMOFF");
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
buffer* VideoDevice::process_capture() {

    if(file_descriptor == -1) {
        return NULL;
    }

    for(;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(file_descriptor, &fds);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int result = select(file_descriptor + 1, &fds, NULL, NULL, &tv);
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

        if(read_frame() == 1) {
            return &frame_buffers[curBufferIndex];
        }
    }

    return NULL;
}

/* Stop capturing, uninitialize the device and free all memory. */
void VideoDevice::stop_device() {
    stop_capture();
    uninit_device();
    close_device();


}

DeviceType VideoDevice::detect_device(const char* dev_name) {
	struct v4l2_capability cap;
	int fd = -1;
	DeviceType dev_type = NO_DEVICE;


	fd = v4l2_open(dev_name);
	if (fd == -1)
	{
		return NO_DEVICE;
	}

	// Get device capabilities
	CLEAR(cap);
	if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		return NO_DEVICE;
	}

	LOGI("Driver detected as: %s", cap.driver);


	if(strncmp((const char*)cap.driver, "usbtv", 5) == 0)
		dev_type = UTV007;
	else if(strncmp((const char*)cap.driver,"em28xx", 6) == 0)
		dev_type = EMPIA;
	else if(strncmp((const char*)cap.driver, "stk1160", 7) == 0)
		dev_type = STK1160;
	else if(strncmp((const char*)cap.driver, "smi2021", 7) == 0)
		dev_type = SOMAGIC;
	else
		dev_type = NO_DEVICE;

	// Close Device
	if(v4l2_close(fd) != SUCCESS_LOCAL) {
		return NO_DEVICE;
	}

	return dev_type;

}

/******** Depricated LibYUV conversion functions**************************************************
 * TODO:  Remove this block at a later time
void VideoDevice::convert_from_yuy2(int index) {

	libyuv::YUY2ToARGB((uint8*)(frame_buffers[index].start), twoByteStride,
	                   rgb_buffer, fourByteStride, device_sets.frame_width, device_sets.frame_height);

	libyuv::ARGBToRGB565(rgb_buffer, fourByteStride, final_buffer, twoByteStride,
	    		device_sets.frame_width, device_sets.frame_height);
}
void VideoDevice::convert_from_uyvy(int index) {

	libyuv::UYVYToARGB((uint8*)(frame_buffers[index].start), twoByteStride,
		                   rgb_buffer, fourByteStride, device_sets.frame_width, device_sets.frame_height);

	libyuv::ARGBToRGB565(rgb_buffer, fourByteStride, final_buffer, twoByteStride,
				device_sets.frame_width, device_sets.frame_height);

}
void VideoDevice::convert_from_rgb565(int index) {

	//
	// TODO:  Check the .length variable, make sure it measures in bytes
	//
	memcpy(final_buffer, frame_buffers[index].start, frame_buffers[index].length);
}
*************************************************************************/


// Helper functions to open and close devices, to keep from repeating code
int v4l2_open(const char* dev_name) {

	struct stat st;
	int fd = - 1;
	if(-1 == stat(dev_name, &st)) {
		LOGE("Cannot identify '%s': %d, %s", dev_name, errno, strerror(errno));
		return ERROR_LOCAL;
	}

	if(!S_ISCHR(st.st_mode)) {
		LOGE("%s is not a valid device", dev_name);
		return ERROR_LOCAL;
	}

	fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
	if(-1 == fd) {
		LOGE("Cannot open '%s': %d, %s", dev_name, errno, strerror(errno));
		if(EACCES == errno) {
			LOGE("Insufficient permissions on '%s': %d, %s", dev_name, errno,
					strerror(errno));
		}
		return ERROR_LOCAL;
	}

	return fd;
}

int v4l2_close(int fd) {
	int result = SUCCESS_LOCAL;
	if(-1 != fd && -1 == close(fd)) {
		result = errnoexit("close");
	}
	fd = -1;
	return result;
}

