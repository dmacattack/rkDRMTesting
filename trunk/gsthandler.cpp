#include "gsthandler.hpp"
#include <QDebug>
#include <gst/app/gstappsrc.h>

// static initialization
gsthandler *gsthandler::theGstClass = NULL;

/**
 * @brief gsthandler::gsthandler - ctor
 */
gsthandler::gsthandler(DRMBuffer *pBuf)
: mpPipeline(NULL)
, mpDRMSrc  (NULL)
, mpCaptureTimer(new QTimer())
, mpDRMBuffer(pBuf)
{
    // setup timer
    mpCaptureTimer->setInterval(30); // approx framerate
    QObject::connect( mpCaptureTimer, SIGNAL(timeout()), this, SLOT(onCaptureTimerExp()) );
}

/**
 * @brief gsthandler::~gsthandler - dtor
 */
gsthandler::~gsthandler()
{
    delete mpCaptureTimer;
}

/**
 * @brief gsthandler::start - launch a video sink pipeline
 *
 * gst-launch-1.0 appsrc name=drmsrc stream-type=0 is-live=true ! \
 *         caps="video/x-raw,width=768,height=1280,format=BGRA,framerate=0/1" ! \
 *    videoflip method=counterclockwise ! \
 *    videocrop top=48 ! \
 *    videoconvert ! \
 *    jpegenc ! \
 *    rtpjpegpay ! \
 *    rtpstreampay ! \
 *    tcpserversink port=7001
 *
 */
void gsthandler::start()
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoflip     = gst_element_factory_make("videoflip",     NULL    );
    GstElement *videocrop     = gst_element_factory_make("videocrop",     NULL    );
    GstElement *videoconvert  = gst_element_factory_make("videoconvert",  NULL    );
    GstElement *vccaps        = gst_element_factory_make("capsfilter",    NULL    );
    GstElement *jpegenc       = gst_element_factory_make("jpegenc",       NULL    );
    GstElement *jpegpay       = gst_element_factory_make("rtpjpegpay",    NULL    );
    GstElement *streampay     = gst_element_factory_make("rtpstreampay",  NULL    );
    GstElement *tcpserversink = gst_element_factory_make("tcpserversink", NULL    );

    // setup the caps to the appsrc
    {
        GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                            "width",  G_TYPE_INT, 768,
                                            "height", G_TYPE_INT, 1280,
                                            "framerate", GST_TYPE_FRACTION, 0, 1,
                                            "format", G_TYPE_STRING, "BGRA",
                                            NULL);
        g_object_set(G_OBJECT(mpDRMSrc), "caps", caps, NULL);
        gst_caps_unref(caps);
    }

    // setup the caps before the encoder
    {
        GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, "I420",
                                            NULL);
        g_object_set(G_OBJECT(vccaps), "caps", caps, NULL);
        gst_caps_unref(caps);
    }

    // add elements into the pipeline
    gst_bin_add_many(GST_BIN(mpPipeline),
                     mpDRMSrc,
                     videoflip,
                     videocrop,
                     videoconvert,
                     vccaps,
                     jpegenc,
                     jpegpay,
                     streampay,
                     tcpserversink, NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoflip,
                          videocrop,
                          videoconvert,
                          vccaps,
                          jpegenc,
                          jpegpay,
                          streampay,
                          tcpserversink, NULL);

    // setup the appsrc props
    g_object_set (G_OBJECT (mpDRMSrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  NULL);

    // set the video flip props
    g_object_set (G_OBJECT (videoflip),
                  "method", 3, NULL);


    // set the video crop props
    g_object_set (G_OBJECT (videocrop),
                  "top", 48, NULL);

    // set the tcp server props
    g_object_set (G_OBJECT (tcpserversink),
                  "port", 7001, NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);

    // start the timer
    mpCaptureTimer->start();

}

/**
 * @brief gsthandler::onCaptureTimerExp - timer slot to send another frame
 */
void gsthandler::onCaptureTimerExp()
{
    // get the frame buffer
    gpointer buf = (gpointer)mpDRMBuffer->buffer();
    guint size = mpDRMBuffer->size();

    // push the frame
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *buffer = NULL;

    // make a buffer
    buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, // flags
                                          buf,               // data
                                          size,              // maxsize
                                          0,                 // offset
                                          size,              // size
                                          NULL,              // user_data
                                          NULL );            // notify

    // push the buffer
    ret = gst_app_src_push_buffer( GST_APP_SRC(mpDRMSrc), buffer );

    if (ret != GST_FLOW_OK)
    {
        qCritical() << "push error " << ret;
    }

}
