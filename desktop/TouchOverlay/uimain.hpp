#ifndef UIMAIN_HPP
#define UIMAIN_HPP

#include <QObject>
#include "tcpclient.hpp"

/**
 * @brief The UIMain class - main ui arbiter class
 */
class UIMain
: public QObject
{
    Q_OBJECT
public:
    UIMain(QObject *pQml);
    ~UIMain();
    void init();

signals:
public slots:
private slots:
    void onButtonClick(int id);
    void updateTcpStatus(TCP::eCONN_STATUS status);

private:


private:
    TCPClient *mpTcpClient;
    QObject *mpQmlObject;
    TCP::eCONN_STATUS mTcpStatus;
};

#endif // UIMAIN_HPP
