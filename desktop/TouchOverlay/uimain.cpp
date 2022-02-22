#include "uimain.hpp"
#include <QDebug>

namespace
{
    enum eBTN_ID
    {
        eBTN_CONNECT = 111,
    };

    const char* QML_PROP_BTN_TXT = "btnTxt";
}

/**
 * @brief UIMain::UIMain - ctor
 */
UIMain::UIMain(QObject *pQml)
: mpTcpClient(new TCPClient())
, mpQmlObject(pQml)
, mTcpStatus(TCP::eCONN_DISCONN)
{

}

/**
 * @brief UIMain::~UIMain - dtor
 */
UIMain::~UIMain()
{
    delete mpTcpClient;
}

/**
 * @brief UIMain::init - init the ui main class
 */
void UIMain::init()
{
    // connect signal/slots
    QObject::connect(mpQmlObject, SIGNAL(buttonClick(int)),                   this,        SLOT(onButtonClick(int)) );
    QObject::connect(mpQmlObject, SIGNAL(moveMouse(int,int)),                 mpTcpClient, SLOT(sendMousePress(int,int)));
    QObject::connect(mpQmlObject, SIGNAL(releaseMouse()),                     mpTcpClient, SLOT(sendMouseRelease()) );
    QObject::connect(mpTcpClient, SIGNAL(updateTcpStatus(TCP::eCONN_STATUS)), this,        SLOT(updateTcpStatus(TCP::eCONN_STATUS)) );
}

/**
 * @brief UIMain::onButtonClick - slot for button click callbacks
 * @param id
 */
void UIMain::onButtonClick(int id)
{
    qDebug() << "clicked button " << id;
    if (id == eBTN_CONNECT)
    {
        if (mTcpStatus == TCP::eCONN_DISCONN)
        {
            mpTcpClient->connectToDevice();
        }
        else if (mTcpStatus == TCP::eCONN_CONNECTED)
        {
            mpTcpClient->disconnectDevice();
        }
    }
}

/**
 * @brief UIMain::updateTcpStatus - slot to update the socket connection status
 */
void UIMain::updateTcpStatus(TCP::eCONN_STATUS status)
{
    QString strStatus = (status == TCP::eCONN_CONNECTED ? "Disconnect" :
                        (status == TCP::eCONN_DISCONN   ? "Connect" : "Connecting" ));

    mpQmlObject->setProperty(QML_PROP_BTN_TXT, strStatus);
}

