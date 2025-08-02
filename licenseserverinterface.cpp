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

    QUrl url(settings.licenseServerUrl);

    if (!url.isValid())
    {
        qWarning() << "Invalid license server URL:" << settings.licenseServerUrl;
        return {};
    }

    QString host = url.host();
    int port = url.port(443);  // default TLS port

    // Connessione cifrata TLS
    socket.connectToHostEncrypted(host, port);

    if (!socket.waitForEncrypted(3000))
    {
        qWarning() << "TLS connection failed:" << socket.errorString();
        return {};
    }

    // Header di comando
    QByteArray payload;
    payload.append(static_cast<char>(0x01)); // CMD_VALIDATE_TOKEN
    payload.append(activationKey);           // 32 byte
    payload.append(incompleteToken);         // token C++ serializzato (raw)

    // Invia il payload
    socket.write(payload);

    if (!socket.waitForBytesWritten(2000))
    {
        qWarning() << "Failed to send token validation request:" << socket.errorString();
        return {};
    }

    if (!socket.waitForReadyRead(3000))
    {
        qWarning() << "No response from license server.";
        return {};
    }

    QByteArray response = socket.readAll();
    socket.disconnectFromHost();

    return response;
}

