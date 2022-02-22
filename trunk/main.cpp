#include <QCoreApplication>
#include <QDebug>
#include "drmcapture.hpp"
#include "utility/cmdoptions.hpp"
#include "filehandler.hpp"
#include <gst/gst.h>
#include "gsthandler.hpp"

// Works on Rockchip systems but fail with ENOSYS on AMDGPU
int main(int argc, char *argv[])
{
    qDebug() << "v18.02.2022";

    // init gstreamer
    gst_init(&argc, &argv);

    // start the core application
    QCoreApplication qCoreApp(argc, argv);

    // provide cmd line args to the cmdOptions class TODO
    CmdOptions::setCmdOptions(qCoreApp);

    // init the capture class
    DRMCapture *pCapture = new DRMCapture();
    DRMBuffer *pBuf = pCapture->capture();
    if (pBuf == NULL)
    {
        qCritical() << "DRM Buffer is not accessible. Exiting";
        qCoreApp.exit(0);
    }
    else if (!pBuf->isValid())
    {
        qCritical() << "DRM buffer is not valid. Exiting";
        qCoreApp.exit(0);
    }
    else
    {
        gsthandler *pGst = new gsthandler(pBuf);
        pGst->start();
    }

    //delete pBuf;

    return qCoreApp.exec();
}
