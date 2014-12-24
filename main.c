/* ============================================================================
 * @File: 	 main.c
 * @Author: 	 Ozgur Eralp [ozgur.eralp@outlook.com]
 * @Description: Baby Watch - RTSP Streamer TEST Application 
 * 		 based on GStreamer and running on ARM Platforms
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
 *
 * THIS IS A TEST APPLICATION IMPLEMENTED BY CAMMODULE AND RTSPMODULE
 * ASSUMPTION(S):
 *	- Camera able to capture YUV (UYVY) format.
 *	- GStreamer 0.10 installed.
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <asm/errno.h>
#include "cammodule.h"
#include "rtspmodule.h"

struct t_arguments {
    int width;
    int height;
};

void* t_cammodule_interface (void *arg);
void* t_rtspmodule_interface(void *arg);

sem_t rtsp_ready;

int main(int argc, char *argv[])
{
	int err = 0;
	pthread_t tid1, tid2;
    	struct t_arguments dim;

	dim.width = 640;
	dim.height = 480;

	err = pthread_create(&tid1, NULL, &t_rtspmodule_interface, (void *)&dim);	
	err = pthread_create(&tid2, NULL, &t_cammodule_interface, (void *)&dim);
	err = pthread_join(tid1,NULL);
	err = pthread_join(tid2,NULL);
	if (err != 0)
		printf(">$$ Thread Initilization Error\n");

	printf("\n.Good Bye!.\n\n");

	return 0;
}

/* ============================================================================
 * @Function: 	 t_cammodule_interface
 * @Description: Interface to CAMMODULE
 * ============================================================================
 */
void* t_cammodule_interface(void *arg)
{
	struct cammodule_arguments camarg;
	struct t_arguments* dim = (struct t_arguments*) arg;

	/* Initialize CAMMODULE */
    	camarg.width = dim->width;
	camarg.height = dim->height;
    	camarg.device_name = (char *)"/dev/video0";
	cammodule_init(&camarg);

	/* Start CAMMODULE */
	cammodule_start();

	/* Get video frame(s) */
	sem_wait(&rtsp_ready);	
	int count = 0;
	int size = (camarg.width)*(camarg.height)*4;
	char * fdata = malloc ((sizeof(char))*size);
	while (count < 25000)  //Around 15 minutes if assume 25fps
   	{
		count++;
		cammodule_getframe(fdata);

		if (count != 1)
			sem_wait(&gdata_wait);			
		rtspmodule_setdata(fdata);
		sem_post(&gdata_ready);

		usleep(20000);
	}

	/* Stop CAMMODULE */
	cammodule_stop();

	return 0;
}

/* ============================================================================
 * @Function: 	 t_rtspmodule_interface
 * @Description: Interface to RTSPMODULE
 * ============================================================================
 */
void* t_rtspmodule_interface(void *arg)
{
	struct rtspmodule_arguments rtsparg;
	struct t_arguments* dim = (struct t_arguments*) arg;	

	/* Initialize RTSPMODULE */
	rtsparg.width = dim->width;
	rtsparg.height = dim->height;
	rtsparg.gfps = 10;
	rtsparg.gbitrate = 286;
	rtsparg.gmtu = 704;
	rtsparg.vsrc = (char *)"appsrc";
	rtsparg.vencoder = (char *)"x264enc";
	rtsparg.rtpencoder = (char *)"rtph264pay";
	rtspmodule_init(&rtsparg);
	sem_post(&rtsp_ready);

	/* Start RTSPMODULE */
	rtspmodule_start();

	/* Close RTSPMODULE */
	rtspmodule_close();

	return 0;
}

