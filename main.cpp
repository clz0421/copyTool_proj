#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "diskmanager.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    //以下三个set是为了消除在使用QtQuick.Dialogs模块的时候，消除qml层的警告
    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setOrganizationDomain("mycompany.com");
    QCoreApplication::setApplicationName("MyApp");

    DiskManager *diskManager = new DiskManager(&app); 
    QQmlApplicationEngine engine;
    // 把 diskManager 以 “diskManager” 的名字注入到 QML 全局上下文
    engine.rootContext()->setContextProperty(QStringLiteral("diskManager"), diskManager);
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    if (!engine.load(url)) {
        qCritical() << "QML load errors:" << engine.errors();
        return -1;
    }
    // QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
    //                  &app, [url](QObject *obj, const QUrl &objUrl) {
    //     if (!obj && url == objUrl)
    //         QCoreApplication::exit(-1);
    // }, Qt::QueuedConnection);
    // engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML file.";
        return -1;
    }
    return app.exec();
}

