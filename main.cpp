#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QWebChannel>
#include "databridge.h"
#include "WebSocketTransport.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HumServer3_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QWebSocketServer server(QStringLiteral("QWebChannel Server"),
                            QWebSocketServer::NonSecureMode);

    if (!server.listen(QHostAddress::Any, 12345)) {
        qCritical() << "Impossibile avviare WebSocketServer sulla porta 12345";
        return 1;
    }

    qDebug() << "Server WebSocket in ascolto su ws://localhost:12345";

    QWebChannel *channel = new QWebChannel();
    DataBridge *bridge = new DataBridge();
    channel->registerObject(QStringLiteral("humBridge"), bridge);

    QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
        QWebSocket *socket = server.nextPendingConnection();
        qDebug() << "Nuova connessione WebSocket";

        auto *transport = new WebSocketTransport(socket);
        QObject::connect(socket, &QWebSocket::disconnected, transport, &QObject::deleteLater);
        channel->connectTo(transport);
    });


    return a.exec();
}
