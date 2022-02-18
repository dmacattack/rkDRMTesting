#ifndef GSTHANDLER_HPP
#define GSTHANDLER_HPP

#include <gst/gst.h>
#include <QObject>
#include <QTimer>
#include "drmbuffer.hpp"

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
    GstElement *mpPipeline;
    GstElement *mpDRMSrc;
    QTimer *mpCaptureTimer;
    DRMBuffer *mpDRMBuffer;
    // static elements
    static gsthandler *theGstClass;

};

#endif // GSTHANDLER_HPP
