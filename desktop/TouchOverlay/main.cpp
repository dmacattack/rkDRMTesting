#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "uimain.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    auto qmlMain = engine.rootObjects().first();
    UIMain *pMain = new UIMain(qmlMain);
    pMain->init();

    return app.exec();
}
