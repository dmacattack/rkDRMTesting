#include "gsthandler.hpp"
#include <QDebug>
#include <gst/app/gstappsrc.h>
#include "utility/cmdoptions.hpp"

namespace
{
    const int DEFAULT_FPS = 10;
}

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
, mFrameRate(DEFAULT_FPS)
{
    // setup timer
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

 */
void gsthandler::start()
{
    // set the framerate
    mFrameRate = CmdOptions::frameRate();
    mpCaptureTimer->setInterval(1000/mFrameRate);

    // get the outputfilename
    QString filepath = CmdOptions::outputFilename();

    // get the output format
    QString fmt = CmdOptions::outputFormat();

    // get the output rotation for images
    int degrees = CmdOptions::imageRotation();
    GST_HANDLER::eGST_ROTATION rotation = (degrees == 0   ? GST_HANDLER::eROT_90  :
                                          (degrees == 180 ? GST_HANDLER::eROT_180 :
                                          (degrees == 270 ? GST_HANDLER::eROT_270 : GST_HANDLER::eROT_NONE )));

    // get the streaming parameters
    QString streamIp = CmdOptions::outputIPAddr();

    // print out the params
    qDebug() << "selected params:";
    qDebug() << "   format = " << fmt;
    qDebug() << "   image rotation = " << degrees;
    qDebug() << "   image filepath = " << filepath;
    qDebug() << "   streaming framerate = " << mFrameRate;
    qDebug() << "   streaming ip = " << streamIp;

    // help out the user
    fmt = fmt.toLower();

    // do the right thing
    if (fmt == "raw")
    {
        qDebug() << "**** Saving Raw Image ****";
        // save a raw image
        startRawPipeline(filepath, rotation);
    }
    else if ((fmt == "jpeg") || (fmt == "jpg"))
    {
        qDebug() << "**** Saving JPEG Image ****";
        // save to a jpeg
        startJpegPipeline(filepath, rotation);
    }
    else if (fmt == "png")
    {
        qDebug() << "**** Saving PNG Image ****";
        // save to a png
        startPngPipeline(filepath, rotation);
    }
    else if (fmt == "adb")
    {
        qDebug() << "**** Streaming over ADB ****";
        // stream over adb
        QStringList ip = streamIp.split(":");
        QString addr = ip.first();
        int port = ip.last().toInt();
        startAdbPipeline(addr, port);
    }
    else if (fmt == "udp")
    {
        qDebug() << "**** Streaming over UDP ****";
        // stream over udp
        startUdpPipeline(streamIp);
    }
    else
    {
        qFatal("invalid format given. Exiting");
    }
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

/**
 * @brief gsthandler::startRawPipeline - start a pipeline to a raw file
 * @param location - where to save the file
 * @param rotation - how much to rotate
 *
 * gst-launch-1.0 appsrc name=drmsrc stream-type=0 is-live=true num-buffers=1 ! \
 *         caps="video/x-raw,width=768,height=1280,format=BGRA,framerate=0/1" ! \
 *    videoflip method=<rotation> ! \
 *    filesink location=<location>
 *
 */
void gsthandler::startRawPipeline(const QString &location, const int rotation)
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoflip     = gst_element_factory_make("videoflip",     NULL    );
    GstElement *filesink      = gst_element_factory_make("filesink",      NULL    );

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

    // add elements into the pipeline
    gst_bin_add_many(GST_BIN(mpPipeline),
                     mpDRMSrc,
                     videoflip,
                     filesink,
                     NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoflip,
                          filesink,
                          NULL);

    // setup the appsrc props
    g_object_set (G_OBJECT (mpDRMSrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  "num-buffers", 1,
                  NULL);

    // set the video flip props
    g_object_set (G_OBJECT (videoflip),
                  "method", rotation, NULL);

    // set the filesink props
    g_object_set (G_OBJECT (filesink),
                  "location", location.toStdString().c_str(), NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);
}

/**
 * @brief gsthandler::startJpegPipeline - start a pipeline to a jpeg file
 * @param location - where to save the file
 * @param rotation - how much to rotate
 *
 * gst-launch-1.0 appsrc name=drmsrc stream-type=0 is-live=true num-buffers=1 ! \
 *         caps="video/x-raw,width=768,height=1280,format=BGRA,framerate=0/1" ! \
 *    videoflip method=<rotation> ! \
 *    videoconvert ! \
 *    video/x-raw,format=I420 ! \
 *    jpegenc ! \
 *    filesink location=<location>
 *
 */
void gsthandler::startJpegPipeline(const QString &location, const int rotation)
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoflip     = gst_element_factory_make("videoflip",     NULL    );
    GstElement *videoconvert  = gst_element_factory_make("videoconvert",  NULL    );
    GstElement *vccaps        = gst_element_factory_make("capsfilter",    NULL    );
    GstElement *jpegenc       = gst_element_factory_make("jpegenc",       NULL    );
    GstElement *filesink      = gst_element_factory_make("filesink",      NULL    );

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
                     videoconvert,
                     vccaps,
                     jpegenc,
                     filesink,
                     NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoflip,
                          videoconvert,
                          vccaps,
                          jpegenc,
                          filesink,
                          NULL);

    // setup the appsrc props
    g_object_set (G_OBJECT (mpDRMSrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  "num-buffers", 1,
                  NULL);

    // set the video flip props
    g_object_set (G_OBJECT (videoflip),
                  "method", rotation, NULL);

    // set the filesink props
    g_object_set (G_OBJECT (filesink),
                  "location", location.toStdString().c_str(), NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);
}

/**
 * @brief gsthandler::startPngPipeline - start a pipeline to a png file
 * @param location - where to save the file
 * @param rotation - how much to rotate
 *
 * gst-launch-1.0 appsrc name=drmsrc stream-type=0 is-live=true num-buffers=1 ! \
 *         caps="video/x-raw,width=768,height=1280,format=BGRA,framerate=0/1" ! \
 *    videoflip method=<rotation> ! \
 *    videoconvert ! \
 *    video/x-raw,format=I420 ! \
 *    pngenc ! \
 *    filesink location=<location>
 *
 */
void gsthandler::startPngPipeline(const QString &location, const int rotation)
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoflip     = gst_element_factory_make("videoflip",     NULL    );
    GstElement *videoconvert  = gst_element_factory_make("videoconvert",  NULL    );
    GstElement *vccaps        = gst_element_factory_make("capsfilter",    NULL    );
    GstElement *pngenc        = gst_element_factory_make("pngenc",        NULL    );
    GstElement *filesink      = gst_element_factory_make("filesink",      NULL    );

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

    // add elements into the pipeline
    gst_bin_add_many(GST_BIN(mpPipeline),
                     mpDRMSrc,
                     videoflip,
                     videoconvert,
                     vccaps,
                     pngenc,
                     filesink,
                     NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoflip,
                          videoconvert,
                          vccaps,
                          pngenc,
                          filesink,
                          NULL);

    // setup the appsrc props
    g_object_set (G_OBJECT (mpDRMSrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  "num-buffers", 1,
                  NULL);

    // set the video flip props
    g_object_set (G_OBJECT (videoflip),
                  "method", rotation, NULL);

    // set the filesink props
    g_object_set (G_OBJECT (filesink),
                  "location", location.toStdString().c_str(), NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);
}


/**
 * @brief gsthandler::startAdbPipeline - start a pipeline broadcasting to the adb port
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
 * @param host - tcp server host
 * @param port - tcp port
 */
void gsthandler::startAdbPipeline(const QString &host, int port)
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoconvert  = gst_element_factory_make("videoconvert",  NULL    );
    GstElement *vccaps        = gst_element_factory_make("capsfilter",    NULL    );
    GstElement *jpegenc       = gst_element_factory_make("jpegenc",       NULL    );
    GstElement *tcpserversink = gst_element_factory_make("tcpserversink", NULL    );

    // setup the caps to the appsrc
    {
        GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                            "width",  G_TYPE_INT, 768,
                                            "height", G_TYPE_INT, 1280,
                                            "framerate", GST_TYPE_FRACTION, 0, 1,
                                            "format", G_TYPE_STRING, "BGRA",
                                            "block", G_TYPE_BOOLEAN, TRUE,
                                            NULL);
        g_object_set(G_OBJECT(mpDRMSrc), "caps", caps, NULL);
        gst_caps_unref(caps);
    }

    // setup the caps before the encoder to ensure the jpegencoder is good
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
                     videoconvert,
                     vccaps,
                     jpegenc,
                     tcpserversink, NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoconvert,
                          vccaps,
                          jpegenc,
                          tcpserversink, NULL);

    // setup the appsrc props
    g_object_set (G_OBJECT (mpDRMSrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  "do-timestamp", TRUE,
                  NULL);

    // set the tcp server props
    g_object_set (G_OBJECT (tcpserversink),
                  "port", port,
                  "host", host.toStdString().c_str(),
                  NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);

    // start the timer
    mpCaptureTimer->start();
}

/**
 * @brief gsthandler::startUdpPipeline - start a pipeline broadcasting to a udp port
 * @param clients
 */
void gsthandler::startUdpPipeline(const QString &clients)
{
    mpPipeline                = gst_pipeline_new        ("pipeline"               );
    mpDRMSrc                  = gst_element_factory_make("appsrc",        "drmsrc");
    GstElement *videoflip     = gst_element_factory_make("videoflip",     NULL    );
    GstElement *videocrop     = gst_element_factory_make("videocrop",     NULL    );
    GstElement *videoconvert  = gst_element_factory_make("videoconvert",  NULL    );
    GstElement *vccaps        = gst_element_factory_make("capsfilter",    NULL    );
    GstElement *jpegenc       = gst_element_factory_make("jpegenc",       NULL    );
    GstElement *jpegpay       = gst_element_factory_make("rtpjpegpay",    NULL    );
    GstElement *udpsink       = gst_element_factory_make("udpsink",       NULL    );

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
                     udpsink      , NULL);

    // link the elements
    gst_element_link_many(mpDRMSrc,
                          videoflip,
                          videocrop,
                          videoconvert,
                          vccaps,
                          jpegenc,
                          jpegpay,
                          udpsink      , NULL);

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

    // set the udp server props
    g_object_set (G_OBJECT (udpsink      ),
                  "clients", clients.toStdString().c_str(), NULL);

    qDebug() << "play the pipeline";
    gst_element_set_state(mpPipeline, GST_STATE_PLAYING);

    // start the timer
    mpCaptureTimer->start();
}
