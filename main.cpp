#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QWebChannel>
#include <QHttpServer>
#include <QTcpServer>
#include <QSqlDatabase>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "databridge.h"
#include "websockettransport.h"
#include "settings.h"
#include "SystemKeyStore.h"
#include "ControllerInterface.h"
#include "LicenseServerInterface.h"

#ifdef Q_OS_WIN
 #include <windows.h>
 #include <io.h>
 #include <fcntl.h>
#endif //Q_OS_WIN




LogLevels_t logLevel = HUM_LOG_ALL;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

 #ifdef Q_OS_WIN
    // Forza allocazione console per debug
    AllocConsole();

    // Redirect streams alla console
    FILE* pCout;
    FILE* pCin;
    FILE* pCerr;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCin, "CONIN$", "r", stdin);
    freopen_s(&pCerr, "CONOUT$", "w", stderr);

    // Sincronizza Qt debug con console
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg) {
        printf("%s\n", msg.toLocal8Bit().constData());
        fflush(stdout);
    });
#endif //Q_OS_WIN



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
    bool unregistered = true;
    QByteArray humToken = systemStore.getToken();
    MYDEBUG << "humToken =" << humToken;
    if (humToken.isEmpty())
    {
        MYWARNING << "Product registration in progress, internet connection required";
        humToken = systemStore.getTempToken();
    }
    else
    {
        newDevice = false;
        unregistered = false;
    }

    // === Create controller interface ===
    ControllerInterface* ctrlIf = new ControllerInterface(settings, &app);
    if (ctrlIf)
    {
        QThread::msleep(10);
        QString serialID = ctrlIf->getSerialNumber();
        if (serialID.isNull())
        {
            MYCRITICAL << "Unable to get a valid serial number from controller, aborting.";
            return 1;
        }

        if(newDevice)
        {
            //Registering new controller device
            systemStore.createTempToken(serialID);

            // Request activation key from CLI
            QByteArray activationKeyBytes;
            bool validKey = false;

            while (!validKey)
            {
                MYINFO << "Enter activation key (64 hex characters): ";
                //out.flush();

                // Usa input C standard invece di QTextStream
                char buffer[128];
                if (!fgets(buffer, sizeof(buffer), stdin))
                {
                    MYWARNING << "Error reading input.";
                    continue;
                }

                // Converti a QString e pulisci
                QString hexString = QString::fromLocal8Bit(buffer).trimmed();

                if (hexString.length() != 64)
                {
                    MYWARNING << "Error: activation key must be exactly 64 hex characters.";
                    continue;
                }

                // Verify only hex characters
                bool isValidHex = true;
                for (const QChar &c : hexString)
                {
                    if (!c.isDigit() && (c.toLower() < 'a' || c.toLower() > 'f'))
                    {
                        isValidHex = false;
                        break;
                    }
                }

                if (!isValidHex)
                {
                    MYWARNING << "Error: activation key contains invalid characters. Use only 0-9 and A-F.\n";
                    continue;
                }

                // Convert from hex to bytes
                activationKeyBytes = QByteArray::fromHex(hexString.toUtf8());

                if (activationKeyBytes.size() == 32)
                {
                    validKey = true;
                    MYINFO << "Activation key accepted.\n";
                }
                else
                {
                    MYWARNING << "Error in activation key conversion.\n";
                }
            }

            LicenseServerInterface license(settings);
            humToken = license.requestValidatedToken(activationKeyBytes, systemStore.getTempToken());

            if (humToken.isEmpty())
            {
                MYCRITICAL << "License validation error. System in test mode.";
                unregistered = true;
            }
            else
            {
                systemStore.setToken(humToken);
                settings.activationKeyBytes = activationKeyBytes;
                MYINFO << "New license valid, renewal date: " << systemStore.renewalDate() << ", system fully operational";
                newDevice = false;
                unregistered = false;
            }
        }
        else
        {
            //Current token offline validity check
            if(!systemStore.isTokenStillValid(serialID))
            {
                LicenseServerInterface license(settings);

                //invalid token
                if(!systemStore.isTokenExpired(serialID))
                {
                    MYWARNING << "Token validity expired, internet connection required for renewal";

                    humToken = license.requestValidatedToken(settings.activationKeyBytes, systemStore.getTempToken());
                    if (humToken.isEmpty())
                    {
                        MYCRITICAL << "License validation error. System in test mode.";
                        unregistered = true;
                    }

                    MYINFO << "License renewed";
                    systemStore.setToken(humToken.toHex());
                    unregistered = false;
                }
                else
                {
                    MYWARNING << "This license is invalid. System in test mode.";

                    //I casi sono due:
                    //1-questo PC è stato associato ad un altro controller, cosa che non deve accadere.
                    //2-è stato alterato l'hardware del PC associato, la pedana è la stessa. Contattare
                    // via mail per richiedere una nuova activation key
                    unregistered = true;
                }
            }
            else
            {
                MYINFO << "License valid, renewal date: " << systemStore.renewalDate() << ", system fully operational";
                unregistered = false;
            }
        }
    }
    else
    {
        MYCRITICAL << "Unable to open serial port, aborting.";
        return 1;
    }


    // === Database connection
    MYDEBUG << "Available SQL drivers:" << QSqlDatabase::drivers();

    if (!QSqlDatabase::isDriverAvailable("QMARIADB" /* "QMYSQL" */))
    {
        MYCRITICAL << "QMYSQL driver is not available, System in test mode.!";
        unregistered = true;
    }

    if(!unregistered)
    {



    }

    // ===  Start QWebSocketServer for QWebChannel ===
    QWebSocketServer server(QStringLiteral("QWebChannel Server"),
                            QWebSocketServer::NonSecureMode);
    if (!server.listen(QHostAddress::Any, 12345)) {
        MYCRITICAL << "Failed to start WebSocketServer on port 12345";
        return 1;
    }
    MYDEBUG << "WebSocket server listening on ws://<host>:12345";

    QWebChannel *channel = new QWebChannel();
    DataBridge *bridge = new DataBridge();
    channel->registerObject(QStringLiteral("humBridge"), bridge);

    QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
        QWebSocket *socket = server.nextPendingConnection();
        MYDEBUG << "New WebSocket connection";

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
        MYCRITICAL << "Failed to start QTcpServer on port 8080";
        return 1;
    }

    if (!httpServer.bind(tcpServer)) {
        MYCRITICAL << "Failed to bind QHttpServer to QTcpServer";
        return 1;
    }

    MYDEBUG << "HTTP server listening at http://<host>:8080/";

#ifdef Q_OS_WIN
    ShellExecuteA(nullptr, "open", settings.indexPath.toStdString().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(Q_OS_LINUX)
    QString cmd = QString("xdg-open \"%1\"").arg(settings.indexPath);
    system(cmd.toStdString().c_str());
#else
    MYWARNING << "Cannot open GUI pages";
#endif
    return app.exec();
}
