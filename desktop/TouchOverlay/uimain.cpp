#include "uimain.hpp"
#include <QDebug>

namespace
{
    enum eBTN_ID
    {
        eBTN_CONNECT,
    };
}

/**
 * @brief UIMain::UIMain - ctor
 */
UIMain::UIMain(QObject *pQml)
: mpTcpClient(new TCPClient())
, mpQmlObject(pQml)
{

}

/**
 * @brief UIMain::~UIMain - dtor
 */
UIMain::~UIMain()
{
    delete mpTcpClient;
}

void UIMain::init()
{
    // connect signal/slots

    QObject::connect(mpQmlObject, SIGNAL(buttonClick(int)),   this, SLOT(onButtonClick(int)) );
    QObject::connect(mpQmlObject, SIGNAL(moveMouse(int,int)), this, SLOT(onMousePress(int,int)) );
    QObject::connect(mpQmlObject, SIGNAL(releaseMouse()),     this, SLOT(onMouseRelease()) );
}

void UIMain::onMousePress(int x, int y)
{
    qDebug() << "mouse move to" << x << y;
}

void UIMain::onMouseRelease()
{
    qDebug() << "mouse release";
}

/**
 * @brief UIMain::onButtonClick - slot for button click callbacks
 * @param id
 */
void UIMain::onButtonClick(int id)
{
    qDebug() << "clicked button " << id;
}
