#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QWebChannel>
#include <QHttpServer>
#include <QTcpServer>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "databridge.h"
#include "WebSocketTransport.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Traduzioni Qt (non cambiate)
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HumServer3_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // === 1. Avvia QWebSocketServer per QWebChannel ===
    QWebSocketServer server(QStringLiteral("QWebChannel Server"),
                            QWebSocketServer::NonSecureMode);
    if (!server.listen(QHostAddress::Any, 12345)) {
        qCritical() << "Impossibile avviare WebSocketServer sulla porta 12345";
        return 1;
    }
    qDebug() << "Server WebSocket in ascolto su ws://<host>:12345";

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

    // === 2. Avvia QHttpServer per servire index.html e qwebchannel.js ===
    QHttpServer httpServer;

    const QString wwwRoot = QDir::currentPath();

    httpServer.route("/", [wwwRoot]() {
        QFile file(wwwRoot + "/index.html");
        if (file.open(QIODevice::ReadOnly))
            return QHttpServerResponse("text/html", file.readAll());
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });

    httpServer.route("/qwebchannel.js", [wwwRoot]() {
        QFile file(wwwRoot + "/qwebchannel.js");
        if (file.open(QIODevice::ReadOnly))
            return QHttpServerResponse("application/javascript", file.readAll());
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });

    httpServer.route("/script.js", [wwwRoot]() {
        QFile file(wwwRoot + "/script.js");
        if (file.open(QIODevice::ReadOnly))
            return QHttpServerResponse("application/javascript", file.readAll());
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });
    // In ascolto su porta 8080 per richieste HTTP
    QTcpServer* tcpServer = new QTcpServer(&a);
    if (!tcpServer->listen(QHostAddress::Any, 8080)) {
        qCritical() << "Impossibile avviare QTcpServer sulla porta 8080";
        return 1;
    }

    if (!httpServer.bind(tcpServer)) {
        qCritical() << "Impossibile fare bind di QHttpServer al QTcpServer";
        return 1;
    }

    qDebug() << "Server HTTP in ascolto su http://<host>:8080/";

    return a.exec();
}
