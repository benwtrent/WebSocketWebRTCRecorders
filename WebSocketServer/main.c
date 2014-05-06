/*
 * Quick and easy websocket server for a backend for Javascript websocket scripts.
 * This is NOT optimal and NOT production code, only utilize this as an example
 * There is NO error checking.
 *
 *  Created on: Mar 28, 2014
 *      Author: ben.w.trent@gmail.com
 *
 * The MIT License (MIT)
 *
 * Copyright (c) <year> <copyright holders>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <libwebsockets.h>
#include <gst/gst.h>
#include <gst/app/gstappbuffer.h>
#include <gst/app/gstappsrc.h>
#include <unistd.h>

static int callback_http1(struct libwebsocket_context * this,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
	return 0;
}

static int callback_http2(struct libwebsocket_context * this,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
  return 0;
}

static int callback_video(struct libwebsocket_context * this,
                                   struct libwebsocket *wsi,
                                   enum libwebsocket_callback_reasons reason,
                                   void *user, void *in, size_t len) {
	switch(reason) {
	case LWS_CALLBACK_ESTABLISHED:
		printf("video stream WS connected\n");
		break;
	case LWS_CALLBACK_RECEIVE: {
		guchar* temp = NULL;
		temp = (guchar*)malloc(len);
		memcpy(temp, in, len);
		GstElement* stream = (GstElement*)libwebsocket_context_user(this);
		GstElement* appsrc = gst_bin_get_by_name(GST_BIN(stream), "AppSrc");
		GstBuffer* buffer = gst_app_buffer_new(temp, len, (GstAppBufferFinalizeFunc)g_free, temp);
		gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
		break;
	}
	default:
		break;
	}
	return 0;
}

static int callback_audio(struct libwebsocket_context * this,
                                   struct libwebsocket *wsi,
                                   enum libwebsocket_callback_reasons reason,
                                   void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
            printf("audio stream WS connected\n");
            break;
        case LWS_CALLBACK_RECEIVE: {
        	gfloat* temp = NULL;
        	temp = (gfloat*)g_malloc(len);
        	memcpy(temp, in, len);
        	GstElement* stream = (GstElement*)libwebsocket_context_user(this);
        	GstElement* appsrc = gst_bin_get_by_name(GST_BIN(stream), "AppSrc");
        	GstBuffer* buffer = gst_app_buffer_new(temp, len, (GstAppBufferFinalizeFunc)g_free, temp);
        	GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
	break;
        }
        default:
            break;
    }

    return 0;
}

static struct libwebsocket_protocols protocolsAudio[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",
        callback_http1,
        0
    },
    {
        "audio-protocol",
        callback_audio,
        0
    },
    {
        NULL, NULL, 0   /* End of list */
    }
};

static struct libwebsocket_protocols protocolsLocalVideo[] = {
	    /* first protocol must always be HTTP handler */
	    {
	        "http-only",   // name
	        callback_http2, // callback
	        0              // per_session_data_size
	    },
	    {
	    		"video-protocol",
	    		callback_video,
	    		0
	    },
	    {
	        NULL, NULL, 0   /* End of list */
	    }
};

void* AudioWebSocketLoop(void *context)
{
	while(1)
	{
		libwebsocket_service((struct libwebsocket_context *)context, 40);
	}
}

void* VideoWebSocketLoop(void *context)
{
	while(1)
	{
		libwebsocket_service((struct libwebsocket_context *)context, 50);
	}
}

int main(void) {
    // server url will be ws://<ipaddress on the interface>:9000 for audio, 9002 for video
	struct libwebsocket_context *Audiocontext;
	struct libwebsocket_context *Videocontext;


	gst_init(NULL, NULL);
    pthread_t Audiothread, Videothread;
    GstElement* audiopipeline;
    GstElement* videopipeline;
    GMainLoop* loop;
    struct lws_context_creation_info Audioinfo, Videoinfo;
    GError* error=NULL;

    //change multiudpclient destinations to send the jpeg stream or the raw float stream.
    gchar* audiopipelinestr = g_strdup_printf ("appsrc is-live=true do-timestamp=true name=AppSrc ! audio/x-raw-float, channels=1, rate=44100, endianness=1234, width=32 ! audioconvert ! audio/x-raw-int, depth=16, width=16, rate=44100, channels=1, signed=true, endianness=1234 ! queue ! level ! multiudpsink clients=127.0.0.1:7000 sync=false async=false");
    gchar* videostr = g_strdup_printf("appsrc is-live=true do-timestamp=true name=AppSrc ! image/jpeg, height=240, width=320, framerate=10/1 ! multipartmux ! multiudpsink clients=127.0.0.1:7002 sync=false async=false");

    /**
     * Example client pipelines
     * udpsrc port=7000 caps="audio/x-raw-int, depth=16, width=16, endianness=1234, channels=1, signed=true, rate=44100 ! autoaudiosink sync=false async=false
     * udpsrc port=7002 caps="image/jpeg, height=240, width=360, framerate=10/1" ! jpegparse ! jpegdec ! ffmpegcolorspace ! autovideosink sync=false async=false
     */

    audiopipeline = gst_parse_launch(audiopipelinestr, &error);
    videopipeline = gst_parse_launch(videostr, &error);

    loop = g_main_loop_new(NULL, FALSE);
    memset(&Audioinfo, 0, sizeof Audioinfo);
    memset(&Videoinfo, 0, sizeof Videoinfo);

    Audioinfo.port = 9000;
    Audioinfo.iface = "lo"; //change to eth0 or external NIC if external access is needed.
    Audioinfo.ssl_ca_filepath = NULL;
    Audioinfo.ssl_private_key_filepath = NULL;
    Audioinfo.protocols = protocolsAudio;
    Audioinfo.options = 0;
    Audioinfo.extensions = libwebsocket_get_internal_extensions();
    Audioinfo.gid = -1;
    Audioinfo.uid = -1;
    Audioinfo.user = (void*)audiopipeline;

    Videoinfo.port = 9002;
    Videoinfo.iface = "lo"; //same here as above
    Videoinfo.ssl_ca_filepath = NULL;
    Videoinfo.ssl_private_key_filepath = NULL;
    Videoinfo.protocols = protocolsLocalVideo;
    Videoinfo.options = 0;
    Videoinfo.extensions = libwebsocket_get_internal_extensions();
    Videoinfo.gid = -1;
    Videoinfo.uid = -1;
    Videoinfo.user = (void*)videopipeline;

    Audiocontext = libwebsocket_create_context(&Audioinfo);
    Videocontext = libwebsocket_create_context(&Videoinfo);

    printf("starting server...\n");

    gst_element_set_state(audiopipeline, GST_STATE_PLAYING);
    gst_element_set_state(videopipeline, GST_STATE_PLAYING);

    pthread_create(&Audiothread, NULL, AudioWebSocketLoop, (void*)Audiocontext);
    pthread_create(&Videothread, NULL, VideoWebSocketLoop, (void*)Videocontext);

    pthread_join(Videothread, NULL);
    pthread_join(Audiothread, NULL);
    g_main_loop_run(loop);


    libwebsocket_context_destroy(Audiocontext);
    libwebsocket_context_destroy(Videocontext);

    return 0;
}
