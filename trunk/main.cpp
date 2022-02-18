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
    qDebug() << "buf is null" << (pBuf  == NULL);
    if (pBuf && pBuf->isValid())
    {
      //  FileHandler::writeToFile("/mnt/userdata/test4.data", pBuf);
      //
      //  qDebug() << "waiting a few seconds before capturing another";
      //  usleep(5000000); // should be 5s
      //
      //  FileHandler::writeToFile("/mnt/userdata/test5.data", pBuf);
        gsthandler *pGst = new gsthandler(pBuf);
        pGst->start();
    }

    //delete pBuf;

    return qCoreApp.exec();
}
