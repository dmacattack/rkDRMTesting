#include <QCoreApplication>
#include <QDebug>
#include "drmcapture.hpp"
#include "utility/cmdoptions.hpp"

// Works on Rockchip systems but fail with ENOSYS on AMDGPU
int main(int argc, char *argv[])
{
    qDebug() << "v18.02.2022";

    // start the core application
    QCoreApplication qCoreApp(argc, argv);

    // provide cmd line args to the cmdOptions class TODO
    CmdOptions::setCmdOptions(qCoreApp);

    // init the capture class
    DRMCapture *pCapture = new DRMCapture();
    pCapture->capture();

    return qCoreApp.exec();
}
