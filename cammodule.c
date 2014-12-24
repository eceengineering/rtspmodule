/* ============================================================================
 * @File: 	 cammodule.c
 * @Author: 	 Ozgur Eralp [ozgur.eralp@outlook.com]
 * @Description: Camera Interface Application running on ARM Platforms
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/errno.h>
#include "v4l2cam.h"
#include "cammodule.h"

struct capture_info capinfo;

/* ============================================================================
 * @Function: 	 cammodule_init
 * @Description: Initialize V4L2 capture interface.
 * ============================================================================
 */
int cammodule_init (struct cammodule_arguments *arg)
{
    	capinfo.width = arg->width;
	capinfo.height = arg->height;
    	capinfo.device_name = arg->device_name;
	capinfo.fd = -1;

	if (init_camera(&capinfo) == 0)
	{
    		printf("...camera init'ed successfully\n");
		return 0;
	}else{
		printf("$$ camera initialization error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_start
 * @Description: Start V4L2 capture interface.
 * ============================================================================
 */
int cammodule_start (void)
{
	if (start_camera(&capinfo) == 0)
	{
    		printf("...camera capturing started.\n");
		return 0;
	}else{
		printf("$$ camera start error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_stop
 * @Description: Stop V4L2 capture interface.
 * ============================================================================
 */
int cammodule_stop (void)
{
	if (close_camera(&capinfo) == 0)
	{
    		printf("...camera closed.\n");
		return 0;
	}else{
		printf("$$ camera close error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_getframe
 * @Description: Get and copy the video frame into a pointer.
 * ============================================================================
 */
int cammodule_getframe (char *data)
{
	int 	buf_no;
	int	height = capinfo.height;
	int 	width = capinfo.width;
	char 	*srcPlane;

	/*pointer of the frame captured by driver */
    	buf_no = get_camera_frame(&capinfo);
    	srcPlane = capinfo.userptr[buf_no];

    	memcpy(data, srcPlane, height*width*4);
		
	/*release the driver buffer */
    	put_camera_frame(&capinfo, buf_no);

	return 0;
}

