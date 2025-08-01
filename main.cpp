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
#include "websockettransport.h"
#include "settings.h"
#include "SystemKeyStore.h"
#include "ControllerInterface.h"
#include "LicenseServerInterface.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Qt translations (unchanged)
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HumServer3_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
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

    // === Create controller interface ===
    ControllerInterface* ctrlIf = new ControllerInterface(settings, &app);
    if (ctrlIf)
    {
        QString serialID = ctrlIf->getSerialNumber();
        if (serialID.isNull())
        {
            qCritical() << "Unable to get a valid serial number from controller";
            return 1;
        }

        if(newDevice)
        {
            //Registering new controller device
            systemStore.createTempToken(serialID);

            //continuare con connessione al server di licenze inviando il token temporaneo
            //(ovvero con check time di oggi, quindi si comporta come nel caso di token scaduto)
        }
        else
        {
            //Current token offline validity check
            if(!systemStore.isTokenStillValid(serialID))
            {
                LicenseServerInterface license(settings);
                QByteArray tempToken = systemStore.getTempToken();

                //invalid token
                if(!systemStore.isTokenExpired(serialID))
                {
                    qWarning() << "Token validity expired, internet connection required for renewal";
                    //continuare con connessione al server di licenze inviando il token attuale

                    QByteArray validated = license.requestValidatedToken(tempToken, tempToken);
                    if (validated.isEmpty())
                    {
                        qCritical() << "Validation error.";
                        return 1;
                    }

                    qInfo() << "Token renewed";
                    systemStore.setToken(validated.toHex());
                }
                else
                {
                    qWarning() << "This token is invalid";
                    //I casi sono due:
                    //1-questo PC è stato associato ad un altro controller.
                    //Solo i PC di servizio possono collegarsi a tutti i controller
                    //per cui saranno registrati nel server come tali.
                    //I PC di produzione si associano ad un singolo controller per
                    //cui il controller precedentemente associato deve essere dissociato.
                    //2-è stato alterato l'hardware del PC associato, la pedana è la stessa.
                    //inviare il token invalido e un nuovo temp token
                    //al server di licenze per disabilitare la precedente configurazione hw
                    //
                    //Tutte queste operazioni richiedono interventi manuali, non sono automatiche

                    //Registering new controller device
                    QByteArray invalidToken = tempToken;
                    systemStore.createTempToken(serialID);
                    tempToken = systemStore.getTempToken();
                    QByteArray validated = license.requestValidatedToken(invalidToken, tempToken);
                    if (validated.isEmpty())
                    {
                        qCritical() << "Validation error.";
                        return 1;
                    }
                }
            }

            //token valido
        }

    }
    else
    {
        qCritical() << "Unable to open serial port";
        return 1;
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
    QTcpServer* tcpServer = new QTcpServer(&app);
    if (!tcpServer->listen(QHostAddress::Any, 8080)) {
        qCritical() << "Failed to start QTcpServer on port 8080";
        return 1;
    }

    if (!httpServer.bind(tcpServer)) {
        qCritical() << "Failed to bind QHttpServer to QTcpServer";
        return 1;
    }

    qDebug() << "HTTP server listening at http://<host>:8080/";

    return app.exec();
}
