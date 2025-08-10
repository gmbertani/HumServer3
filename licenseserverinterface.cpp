#include "LicenseServerInterface.h"
#include <QHostAddress>
#include <QUrl>
#include <QDataStream>
#include <QDebug>

LicenseServerInterface::LicenseServerInterface(Settings &settingsRef, QObject *parent)
    : QObject(parent), settings(settingsRef)
{
}

QByteArray LicenseServerInterface::requestValidatedToken(const QByteArray &activationKey, const QByteArray &incompleteToken)
{
    QSslSocket socket;

    QString host = settings.licenseServerSite;
    int port = settings.licenseServerPort;

    // Connessione cifrata TLS
    socket.connectToHostEncrypted(host, port);

    if (!socket.waitForEncrypted(3000))
    {
        MYWARNING << "TLS connection to " << host << " : " << port << " failed:" << socket.errorString();
        return {};
    }


    // Ignora errori SSL se stai usando un certificato self-signed
    QObject::connect(&socket, &QSslSocket::sslErrors, [&socket](const QList<QSslError> &errors)
     {
         for (const auto &e : errors)
             MYWARNING << "SSL error:" << e.errorString();
         socket.ignoreSslErrors();
     });

    // Header di comando
    QByteArray payload;
    payload.append('1');                     // CMD_VALIDATE_TOKEN
    payload.append(activationKey);           // 32 byte
    payload.append(incompleteToken);         // token C++ serializzato (raw)

    // Invia il payload
    socket.write(payload);

    if (!socket.waitForBytesWritten(2000))
    {
        MYWARNING << "Failed to send token validation request:" << socket.errorString();
        return {};
    }

    if (!socket.waitForReadyRead(3000))
    {
        MYWARNING << "No response from license server.";
        return {};
    }

    QByteArray response = socket.readAll();
    socket.disconnectFromHost();

    return response;
}

