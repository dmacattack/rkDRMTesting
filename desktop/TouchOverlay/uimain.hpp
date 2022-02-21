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
    void onMousePress(int x, int y);
    void onMouseRelease();
    void onButtonClick(int id);

private:
    TCPClient *mpTcpClient;
    QObject *mpQmlObject;
};

#endif // UIMAIN_HPP
