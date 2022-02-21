#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

#include <QObject>
#include <QTcpSocket>

namespace TCP
{
    struct tTcpDataGram
    {
        int x;
        int y;
        int ev;
    };

    enum eCONN_STATUS
    {
        eCONN_DISCONN,
        eCONN_CONNECTING,
        eCONN_CONNECTED,
    };

    enum eMOUSE_EVENT
    {
        eMOUSE_UNSET   = 0,
        eMOUSE_PRESS   = 1,
        eMOUSE_RELEASE = 2
    };
}


class TCPClient
: public QObject
{
    Q_OBJECT
public:
    TCPClient();
    ~TCPClient();

    void connectToDevice();
    void disconnectDevice();

signals:
    void updateTcpStatus(TCP::eCONN_STATUS);

public slots:
    void sendMousePress(int x, int y);
    void sendMouseRelease();

private slots:
    void onTcpConnect();
    void onTcpDisconnected();

private:
    void sendDgram(TCP::tTcpDataGram dgram);

private:
    QTcpSocket *mpSocket;
};

#endif // TCPCLIENT_HPP
