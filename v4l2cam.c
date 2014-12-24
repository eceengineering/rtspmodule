/* ============================================================================
 * @File: 	 v4l2cam.c
 * @Author: 	 Ozgur Eralp [ozgur.eralp@outlook.com]
 * @Description: Camera V4L2 Interface
 *
 * ============================================================================
 *
 * Copyright 2014 Ozgur Eralp.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ============================================================================
 */

#include <asm/errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include "v4l2cam.h"

/* Macro for clearing structures */
#define CLEAR(x) memset (&(x), 0, sizeof (x))

static int alloc_buffers(unsigned int buf_cnt, enum v4l2_buf_type type,
                 struct capture_info *info);


/* ============================================================================
 * @Function: 	 init_camera
 * @Description: Initialize Kernel camera driver for capturing.
 * ============================================================================
 */
int init_camera(struct capture_info *cinfo)
{
	int 				err = 0;
    	//v4l2_std_id			std_id = V4L2_STD_525_60;
    	struct v4l2_capability      	cap;
    	struct v4l2_format          	fmt;
    	struct v4l2_input          	input;
    	//struct v4l2_control 		control;
    	//struct v4l2_requestbuffers  	req;
    	//enum v4l2_buf_type          	type;

    	printf(".openning capture device %s\n", cinfo->device_name);
    	cinfo->fd = open(cinfo->device_name, O_RDWR, 0);
    	if (cinfo->fd == -1) {
        	printf("$$ Cannot open capture device %s\n", cinfo->device_name);
        	return -ENODEV;
    	}

    	printf(".detecting camera on the capture input \n");
    	memset (&input, 0, sizeof (input));
    	if (-1 == ioctl (cinfo->fd, VIDIOC_ENUMINPUT, &input)) {
    		printf("$$ Cannot detect camera on the capture input\n");
        	err = ENODEV;
        	goto cleanup_devnode;
    	}

	printf(".setting camera as an input\n");    
	input.type = V4L2_INPUT_TYPE_CAMERA;
    	if (ioctl(cinfo->fd, VIDIOC_ENUMINPUT, &input) == -1) {
        	printf("$$Unable to enumerate input\n");
    	}
    
    	err = 0;
	input.type = V4L2_INPUT_TYPE_CAMERA;
	input.index = 0;
    	if (ioctl(cinfo->fd, VIDIOC_S_INPUT, &input) == -1) {
        	printf("$$ Failed set video input\n");
        	err = ENODEV;
        	goto cleanup_devnode;
    	}

    	printf(".asking device for capture capabilities\n");
    	if (ioctl(cinfo->fd, VIDIOC_QUERYCAP, &cap) == -1) {
        	printf("$$ Unable to query device for capture capabilities\n");
        	err = errno;
        	goto cleanup_devnode;
    	}

    	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        	printf("$$ Device does not support capturing\n");
        	err = EINVAL;
        	goto cleanup_devnode;
    	}

    	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        	printf("$$ Device does not support streaming\n");
        	err = EINVAL;
        	goto cleanup_devnode;
    	}


    	printf(".setting format V4L2_PIX_FMT_UYVY\n");
    	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	fmt.fmt.pix.width = cinfo->width;
    	fmt.fmt.pix.height = cinfo->height;
    	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (ioctl(cinfo->fd, VIDIOC_S_FMT, &fmt) == -1) {
		printf("$$ failed to set format on capture device \n");
		err = EINVAL;
        	goto cleanup_devnode;
	}

	if (ioctl(cinfo->fd, VIDIOC_G_FMT, &fmt) == -1) {
		printf("$$ failed to get format from capture device \n");
		err = EINVAL;
        	goto cleanup_devnode;
	} else {
		printf(".capture pitch: bytesperline:%d\n", fmt.fmt.pix.bytesperline);
		printf(".capture pitch: width:%d height:%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
	}

	printf(".allocating capture driver buffers\n");
    	if (alloc_buffers(2, V4L2_BUF_TYPE_VIDEO_CAPTURE, cinfo) < 0) {
        	printf("$$ Unable to allocate capture driver buffers\n");
        	err = ENOMEM;
        	goto cleanup_devnode;
    	}
    	
	return 0;

cleanup_devnode:
	close(cinfo->fd);
	return -err;
}

/* ============================================================================
 * @Function: 	 start_camera
 * @Description: Start Kernel camera driver streaming.
 * ============================================================================
 */
int start_camera(struct capture_info *cinfo)
{
	int 				err = 0;
	enum v4l2_buf_type          	type;

	printf(".starting streaming\n");

    	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(cinfo->fd, VIDIOC_STREAMON, &type) == -1) {
        	printf("$$ VIDIOC_STREAMON failed on device\n");
		err = EPERM;
        	goto cleanup_devnode;
	}

	return 0;

cleanup_devnode:
	close(cinfo->fd);
	return -err;
}

/* ============================================================================
 * @Function: 	 close_camera
 * @Description: Stop capturing from the camera.
 * ============================================================================
 */
int close_camera(struct capture_info *cinfo)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int i;

	/* Stop the video streaming */
    if (ioctl(cinfo->fd, VIDIOC_STREAMOFF, &type) == -1) {
        printf("$$ VIDIOC_STREAMOFF failed on device\n");
    }

    for (i = 0; i < V4L2_MAX_BUFFER_COUNT; i++)
        munmap(cinfo->userptr[i], cinfo->v4l2buf[i].length);

	close(cinfo->fd);
	cinfo->fd = -1;
	return 0;
}


/* ============================================================================
 * @Function:	 get_camera_frame
 * @Description: Receives a frame from the camera driver using V4L2 API.
 * ============================================================================
 */
int get_camera_frame(struct capture_info *cinfo)
{
    	struct v4l2_buffer v4l2buf;

    	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	v4l2buf.memory = V4L2_MEMORY_MMAP;

    	/* Get a frame buffer with captured data */
    	if (ioctl(cinfo->fd, VIDIOC_DQBUF, &v4l2buf) < 0) {
        	printf("$$ VIDIOC_DQBUF failed\n");
        	return -EIO;
    	}

    	return v4l2buf.index;
}


/* ============================================================================
 * @Function:	 put_camera_frame
 * @Description: Gives received camera buffer back to V4L2 kernel driver.
 * ============================================================================
 */
int put_camera_frame(struct capture_info *cinfo, int buf_no)
{
    	cinfo->v4l2buf[buf_no].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    	/* Issue captured frame buffer back to device driver */
    	if (ioctl(cinfo->fd, VIDIOC_QBUF, &cinfo->v4l2buf[buf_no]) == -1) {
       	 	printf("$$ VIDIOC_QBUF failed (%s)\n", strerror(errno));
        	return -EIO;
    	}

    	return 0;
}


/* ============================================================================
 * @Function:	 alloc_buffers
 * @Description: Allocates capture buffers from the kernel driver. Since
 * kernel drivers are contiguous.
 * ============================================================================
 */
static int alloc_buffers(unsigned int buf_cnt, enum v4l2_buf_type type,
                          struct capture_info *info)
{
    	struct v4l2_requestbuffers  req;
    	struct v4l2_format          fmt;

    	fmt.type = type;
    	int fd = info->fd;
    	unsigned int i;
    	//int buf_size = (HEIGHT*WIDTH*4);

    	if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
    		printf("$$ error in alloc_buffers 1\n");
        	return -EINVAL;
    	}

    	req.count  = buf_cnt;
    	req.type   = type;
    	req.memory = V4L2_MEMORY_MMAP;

    	/* Allocate buffers in the capture device driver */
    	if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    		printf("$$ error in alloc_buffers 2\n");
        	return -ENOMEM;
    	}

    	if (req.count < buf_cnt || !req.count) {
    		printf("$$ error in alloc_buffers 3\n");
        	return -ENOMEM;
    	}

    	for (i = 0; i < buf_cnt; i++)
    	{
        	info->v4l2buf[i].type   = type;
        	info->v4l2buf[i].memory = V4L2_MEMORY_MMAP;
        	info->v4l2buf[i].index  = i;
        	//info->v4l2buf[i].length = buf_size;

        	if (ioctl(fd, VIDIOC_QUERYBUF, &info->v4l2buf[i]) == -1) {
            		printf("$$ error in alloc_buffers 4\n");
            		return -ENOMEM;
        	}

        	/* Map the driver buffer to user space */
        	info->userptr[i] = mmap(NULL,
                   	info->v4l2buf[i].length,
                   	PROT_READ | PROT_WRITE,
                   	MAP_SHARED,
                   	fd,
                   	info->v4l2buf[i].m.offset);

        	if (info->userptr[i] == MAP_FAILED) {
            		printf("$$ error in alloc_buffers 5\n");
            		return -ENOMEM;
        	}

        	/* Queue buffer in device driver */
        	if (ioctl(fd, VIDIOC_QBUF, &info->v4l2buf[i]) == -1) {
            		printf("$$ error in alloc_buffers 6\n");
            		return -ENOMEM;
       	 	}
    	}

   	return 0;
}

