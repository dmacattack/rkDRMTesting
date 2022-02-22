#ifndef GSTHANDLER_HPP
#define GSTHANDLER_HPP

#include <gst/gst.h>
#include <QObject>
#include <QTimer>
#include "drmbuffer.hpp"

namespace GST_HANDLER
{
    // rotation matches videoflip method
    enum eGST_ROTATION
    {
        eROT_NONE = 0,
        eROT_90   = 1,
        eROT_180  = 2,
        eROT_270  = 3,

    };
}

/**
 * @brief The gsthandler class - handle the gstreamer pipeline data
 */
class gsthandler
: public QObject
{
    Q_OBJECT
public:
    gsthandler(DRMBuffer *pBuf);
    ~gsthandler();
    void start();

private slots:
    void onCaptureTimerExp();

private:
    void startRawPipeline(const QString &location, const int rotation);
    void startJpegPipeline(const QString &location, const int rotation);
    void startPngPipeline(const QString &location, const int rotation);
    void startImagePipeline(const QString &location);
    void startAdbPipeline(const QString &host, int port);
    void startUdpPipeline(const QString &clients);

private:
    GstElement *mpPipeline;
    GstElement *mpDRMSrc;
    QTimer *mpCaptureTimer;
    DRMBuffer *mpDRMBuffer;
    int mFrameRate;

    // static elements
    static gsthandler *theGstClass;

};

#endif // GSTHANDLER_HPP
