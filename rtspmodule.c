/* ============================================================================
 * @File: 	 rtspmodule.c
 * @Author: 	 Ozgur Eralp [ozgur.eralp@outlook.com]
 * @Description: RTSP Module Implementation based on GStreamer 0.10
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
 * Setup Example:
 * gst-launch videotestsrc ! ffenc_mpeg4 ! rtpmp4vpay send-config=true !
 * udpsink host=192.168.0.114 port=5000
 *
 * .-------.    .-----------.    .----------.     .----------.     .--------.
 * | v4l2  |    |encoder    |    |rtpencoder|     |rtpsession|     |udpsink | RTP
 * |       |    |           |    |          |     |          |     |        |
 * |      src->sink        src->sink       src->rtpsink send_rtp->sink      | port=5000
 * '-------'    '-----------'    '----------'     |          |     '--------'
 *                                                |          |     .--------.
 *                                                |          |     |udpsink | RTCP
 *                                                |          |     |        |
 *                                                |      receive->sink      | port=5001
 *                                                '----------'     '--------'
 *
 * ============================================================================ 
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <glib.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include "rtspmedia.h"
#include "rtspmodule.h"

static GMainLoop  *loop;
static GstBuffer  *databuffer;
static char *inputdatabuffer;
static guint datasize;
static GstElement *pipeline;

static GstRTSPServer *server;
static GstRTSPMediaMapping *mapping;
static GstRTSPMediaFactory *factory;

static gboolean cleanup_timeout(GstRTSPServer * server, gboolean ignored);
static gboolean bus_watch(GstBus *bus, GstMessage *msg, gpointer data);
static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data);
static GstElement* construct_app_pipeline(void);

static struct rtspmodule_arguments arguments;

/* ============================================================================
 * @Function: 	 rtspmodule_init
 * @Description: Initialize GST Application interface.
 * ============================================================================
 */
int rtspmodule_init (struct rtspmodule_arguments *arg)
{
	guint major, minor, micro, nano;
	GstBus  *bus;

	/* Settings - Encoder and Streaming */
	arguments.width = arg->width;
	arguments.height = arg->height;
	arguments.gfps = arg->gfps;
	arguments.gbitrate = arg->gbitrate;
	arguments.gmtu = arg->gmtu;
	arguments.vsrc = arg->vsrc;
	arguments.vencoder = g_strdup(arg->vencoder);
	arguments.rtpencoder = arg->rtpencoder;

	/* GSTAPP Setup */
	gst_init(NULL, NULL);

	gst_version(&major, &minor, &micro, &nano);
	g_print("..This program is linked against GStreamer %d.%d.%d\n", major, minor, micro);

	loop = g_main_loop_new(NULL, FALSE);

	datasize = arguments.height * arguments.width * 4;
	databuffer = gst_buffer_new_and_alloc (datasize);
	inputdatabuffer = malloc ((sizeof(char))*datasize);

	pipeline = construct_app_pipeline();
	if ( !pipeline ) {
		g_printerr("Failed to construct pipeline\n");
		return -1;
	}
	g_print("..GSTAPP Pipeline Setup... \n");

	/* we add a message handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	gst_bus_add_watch(bus, bus_watch, loop);
	gst_object_unref(bus);

	/* create a server instance */
  	server = gst_rtsp_server_new ();

  	/* get the mapping for this server, every server has a default mapper object
   	* that be used to map uri mount points to media factories */
  	mapping = gst_rtsp_server_get_media_mapping (server);

	/* make a media factory for a test stream.
	* The default media factory can use
   	* gst-launch syntax to create pipelines.
   	* any launch line works as long as it contains elements
	* named pay%d. Each element with pay%d names will be a stream */
  	//factory = gst_rtsp_media_factory_new ();
    	factory = GST_RTSP_MEDIA_FACTORY(gst_rtsp_media_factory_custom_new());

  	// allow multiple clients to see the same video
  	gst_rtsp_media_factory_set_shared (factory, TRUE);
    	g_object_set(factory, "bin", pipeline, NULL);

  	/* attach the test factory to the /test url */
  	gst_rtsp_media_mapping_add_factory (mapping, "/bbwatch", factory);
  	/* don't need the ref to the mapping anymore */
  	g_object_unref (mapping);
  	/* attach the server to the default maincontext */
  	gst_rtsp_server_attach (server, NULL);
	 /* add a timeout for the session cleanup */
  	g_timeout_add_seconds (2, (GSourceFunc) cleanup_timeout, server);
	g_print("..GST Pipeline Initialized ...\n");
	
	return 0;
}

/* ============================================================================
 * @Function: 	 rtspmodule_start
 * @Description: Start GST Pipeline.
 * ============================================================================
 */
int rtspmodule_start(void)
{
	/* Set the pipeline to "playing" state*/
	g_print("..Now streaming...\n");
    	g_print ("stream ready at rtsp://127.0.0.1:8554/bbwatch\n");
	g_main_loop_run(loop);

	/* Out of the main loop, clean up nicely */
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT (pipeline));

	return 0;
}

/* ============================================================================
 * @Function: 	 rtspmodule_close
 * @Description: Close GST Pipeline.
 * ============================================================================
 */
int rtspmodule_close(void)
{
	g_main_loop_quit(loop);
	return 0;
}


/* ============================================================================
 * @Function: 	 rtspmodule_setdata
 * @Description: Copy the input data to the buffer.
 * ============================================================================
 */
int rtspmodule_setdata (char *data)
{
	int err = 0;
	
  	memcpy(inputdatabuffer, data, datasize);

	return err;
}


/* ============================================================================
 * @Function: 	 cb_need_data
 * @Description: This callback function feed GST pipeline with video frame.
 * ============================================================================
 */
static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data)
{
	static GstClockTime timestamp = 0;
	GstFlowReturn ret;

	sem_wait(&gdata_ready);
	gst_buffer_set_data(databuffer, inputdatabuffer, datasize);

	GST_BUFFER_TIMESTAMP (databuffer) = timestamp;
	GST_BUFFER_DURATION (databuffer) = gst_util_uint64_scale_int (1, GST_SECOND, arguments.gfps);

	timestamp += GST_BUFFER_DURATION (databuffer);

	g_signal_emit_by_name (appsrc, "push-buffer", databuffer, &ret);

	if (ret != GST_FLOW_OK) {
		/* something wrong, stop pushing */
		g_main_loop_quit (loop);
	}
	sem_post(&gdata_wait);
}


/* ============================================================================
 * @Function: 	 construct_app_pipeline
 * @Description: Pipeline construction, see the diagram drawn at top of the page.
 * ============================================================================
 */
static GstElement* construct_app_pipeline(void)
{
	GstElement *pipeline, *source, *venc, *rtpenc;
	GstCaps *caps;
	gboolean err;
	char *rtpencoder = NULL;

	/* Create gstreamer pipeline */
	pipeline = gst_pipeline_new("gstapp_sender");
	if ( !pipeline ) {
		g_printerr("Failed to create pipeline\n");
		return 0;
	}

	/* Video source initialization */
	source = gst_element_factory_make ("appsrc", "video-source");
	if ( !source ) {
		g_printerr("Failed to create %s\n", arguments.vsrc);
		return 0;
	}
  	g_signal_connect (source, "need-data", G_CALLBACK (cb_need_data), NULL);

	/* Create video encoder */
	venc = gst_element_factory_make(arguments.vencoder, "video-encoder");
	if ( !venc ) {
		g_printerr("Failed to create %s\n", arguments.vencoder);
		return 0;
	}

	//kbits/sec --> bits/sec for H.264 encoder
	if (g_strcmp0(arguments.vencoder, "x264enc") != 0) {
		arguments.gbitrate *= 1024;
	}
	g_object_set(G_OBJECT (venc), "bitrate", arguments.gbitrate, NULL);

	/* Choose RTP encoder according to video codec */
	rtpencoder = g_strdup(arguments.rtpencoder);

	/* Create RTP encoder */
	rtpenc = gst_element_factory_make(rtpencoder, "rtp-encoder");
	if ( !rtpenc ) {
		g_printerr("Failed to create %s\n", rtpencoder);
		return 0;
	}

	g_object_set(G_OBJECT (rtpenc), "name", "pay0", "pt", 96, "mtu", arguments.gmtu, NULL);	
	//g_object_set(G_OBJECT (rtpenc), "name", "pay0", "pt", 96, "send-config", TRUE, NULL);	
	//g_object_set(G_OBJECT (rtpenc), "name", "pay0", "pt", 96, "mtu", arguments.gmtu, "send-config", TRUE, NULL);
	g_free(arguments.vencoder);

	/* Set up the pipeline */
	gst_bin_add_many(GST_BIN (pipeline), source, venc, rtpenc, NULL);

	gchar *capsstr;
	capsstr = g_strdup_printf ("video/x-raw-yuv, format=(fourcc)UYVY, width=(int)%d, height=(int)%d, framerate=%d/1",
					 arguments.width, arguments.height, arguments.gfps);
	caps = gst_caps_from_string (capsstr);
	g_free(capsstr);

	err = gst_element_link_filtered(source, venc, caps);
	gst_caps_unref(caps);
	if ( err==FALSE ) {
		g_printerr("Failed to link source and timeoverlay\n");
		return 0;
	}

	err = gst_element_link_many(venc, rtpenc, NULL);
	if ( err==FALSE ) {
		g_printerr("Failed to link elements\n");
		return 0;
	}

	return pipeline;
}


/* ============================================================================
 * @Function: 	 cleanup_timeout
 * @Description: This timeout is periodically run to clean up the expired
 * sessions from the pool. This needs to be run explicitly currently but might
 * be done automatically as part of the mainloop. 
 * ============================================================================
 */
static gboolean cleanup_timeout(GstRTSPServer * server, gboolean ignored)
{
  	GstRTSPSessionPool *pool;

  	pool = gst_rtsp_server_get_session_pool (server);
  	gst_rtsp_session_pool_cleanup (pool);
  	g_object_unref (pool);

  	return TRUE;
}

/* ============================================================================
 * @Function: 	 bus_watch
 * @Description: This handles the message received from GST. 
 * ============================================================================
 */
static gboolean bus_watch(GstBus *bus, GstMessage *msg, gpointer data) 
{
	GMainLoop *loop = (GMainLoop *) data;

	switch (GST_MESSAGE_TYPE (msg)) 
	{
		case GST_MESSAGE_EOS:
			g_print("End of stream\n");
			g_main_loop_quit(loop);
		break;
		case GST_MESSAGE_ERROR: {
			gchar *debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);

			g_printerr("Error: %s\n", error->message);
			g_error_free(error);

			g_main_loop_quit(loop);
			break;
		}
		default:
			break;
	}

	return TRUE;
}

