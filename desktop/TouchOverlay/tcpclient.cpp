#include "tcpclient.hpp"
#include <QHostAddress>
#include <QDebug>

namespace
{
    const int MOUSE_PORT = 8001;
    const QString IP = "127.0.0.1";
}

/**
 * @brief TCPClient::TCPClient - ctor
 */
TCPClient::TCPClient()
: mpSocket(new QTcpSocket(this))
{
    QObject::connect(mpSocket, SIGNAL(connected()), this, SLOT(onTcpConnect()) );
    QObject::connect(mpSocket, SIGNAL(disconnected()), this, SLOT(onTcpDisconnected()) );
}

/**
 * @brief TCPClient::~TCPClient - dtor
 */
TCPClient::~TCPClient()
{
    disconnectDevice();
    delete mpSocket;
}

/**
 * @brief TCPClient::connectToDevice - connect to the device
 */
void TCPClient::connectToDevice()
{
    emit updateTcpStatus(TCP::eCONN_CONNECTING);
    auto hostaddr = QHostAddress(IP);
    mpSocket->connectToHost(hostaddr, MOUSE_PORT);

    if (mpSocket->waitForConnected())
    {
        qDebug() << "socket connected";
        emit updateTcpStatus(TCP::eCONN_CONNECTED);
    }
    else
    {
        emit updateTcpStatus(TCP::eCONN_DISCONN);
    }
}

/**
 * @brief TCPClient::disconnectDevice
 */
void TCPClient::disconnectDevice()
{
    mpSocket->disconnectFromHost();
}

/**
 * @brief TCPClient::sendMousePress - send a mouse press event to the given coords
 */
void TCPClient::sendMousePress(int x, int y)
{
    TCP::tTcpDataGram dgram;
    dgram.x = x;
    dgram.y = y;
    dgram.ev = TCP::eMOUSE_PRESS;
    sendDgram(dgram);
}

/**
 * @brief TCPClient::sendMouseRelease - send a mouse release event
 */
void TCPClient::sendMouseRelease()
{
    TCP::tTcpDataGram dgram;
    dgram.x = 0;
    dgram.y = 0;
    dgram.ev = TCP::eMOUSE_RELEASE;
    sendDgram(dgram);
}

/**
 * @brief TCPClient::onTcpConnect - slot callback when socket is connected
 */
void TCPClient::onTcpConnect()
{
    emit updateTcpStatus(TCP::eCONN_CONNECTED);
}

/**
 * @brief TCPClient::onTcpDisconnected - slot callback when socket is disconnected
 */
void TCPClient::onTcpDisconnected()
{
    emit updateTcpStatus(TCP::eCONN_DISCONN);
}

/**
 * @brief TCPClient::sendDgram - send a datagram over the socket
 * @param dgram
 */
void TCPClient::sendDgram(TCP::tTcpDataGram dgram)
{
    QByteArray data;
    data.append(dgram.ev);
    data.append(":");
    data.append(dgram.x);
    data.append(":");
    data.append(dgram.y);

    mpSocket->write(data);
}


