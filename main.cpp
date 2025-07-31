#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QWebChannel>
#include <QHttpServer>
#include <QTcpServer>
#include <QSerialPort>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "databridge.h"
#include "websockettransport.h"
#include "settings.h"
#include "SystemKeyStore.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Qt translations (unchanged)
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HumServer3_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // ===  Loading settings ===
    Settings settings;
    settings.load();

    // ===  Load humToken ===
    SystemKeyStore systemStore;
    bool newDevice = true;
    QByteArray humToken = systemStore.getToken();
    qDebug() << "humToken =" << humToken;
    if (humToken.isEmpty())
    {
        qWarning() << "Product registration in progress, internet connection required";
        humToken = systemStore.getTempToken();
    }
    else
    {
        newDevice = false;
    }


    // === Open serial port using settings ===
    QSerialPort *serial = new QSerialPort(&a);

    serial->setPortName(settings.serialPort);

    QStringList parts = settings.serialParams.split(',', Qt::SkipEmptyParts);
    if (parts.size() == 4)
    {
        bool ok = false;

        int baudRate = parts[0].toInt(&ok);
        if (ok)
        {
            serial->setBaudRate(baudRate);
        }
        else
        {
            qWarning() << "Invalid baud rate:" << parts[0];
        }

        int dataBits = parts[1].toInt(&ok);
        if (ok)
        {
            switch (dataBits)
            {
            case 5: serial->setDataBits(QSerialPort::Data5); break;
            case 6: serial->setDataBits(QSerialPort::Data6); break;
            case 7: serial->setDataBits(QSerialPort::Data7); break;
            case 8: serial->setDataBits(QSerialPort::Data8); break;
            default: qWarning() << "Invalid data bits:" << dataBits;
            }
        }

        QString parity = parts[2].trimmed().toLower();
        if (parity == "n") serial->setParity(QSerialPort::NoParity);
        else if (parity == "e") serial->setParity(QSerialPort::EvenParity);
        else if (parity == "o") serial->setParity(QSerialPort::OddParity);
        else qWarning() << "Invalid parity:" << parity;

        int stopBits = parts[3].toInt(&ok);
        if (ok)
        {
            if (stopBits == 1) serial->setStopBits(QSerialPort::OneStop);
            else if (stopBits == 2) serial->setStopBits(QSerialPort::TwoStop);
            else qWarning() << "Invalid stop bits:" << stopBits;
        }
    }
    else
    {
        qWarning() << "Invalid serialParams format, expected: baud,dataBits,parity,stopBits";
    }



    // ===  Start QWebSocketServer for QWebChannel ===
    QWebSocketServer server(QStringLiteral("QWebChannel Server"),
                            QWebSocketServer::NonSecureMode);
    if (!server.listen(QHostAddress::Any, 12345)) {
        qCritical() << "Failed to start WebSocketServer on port 12345";
        return 1;
    }
    qDebug() << "WebSocket server listening on ws://<host>:12345";

    QWebChannel *channel = new QWebChannel();
    DataBridge *bridge = new DataBridge();
    channel->registerObject(QStringLiteral("humBridge"), bridge);

    QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
        QWebSocket *socket = server.nextPendingConnection();
        qDebug() << "New WebSocket connection";

        auto *transport = new WebSocketTransport(socket);
        QObject::connect(socket, &QWebSocket::disconnected, transport, &QObject::deleteLater);
        channel->connectTo(transport);
    });

    // ===  Start QHttpServer to serve index.html, qwebchannel.js, and script.js ===
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

    // Listen on port 8080 for HTTP requests
    QTcpServer* tcpServer = new QTcpServer(&a);
    if (!tcpServer->listen(QHostAddress::Any, 8080)) {
        qCritical() << "Failed to start QTcpServer on port 8080";
        return 1;
    }

    if (!httpServer.bind(tcpServer)) {
        qCritical() << "Failed to bind QHttpServer to QTcpServer";
        return 1;
    }

    qDebug() << "HTTP server listening at http://<host>:8080/";

    return a.exec();
}
