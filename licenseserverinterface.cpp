#include "LicenseServerInterface.h"
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>

LicenseServerInterface::LicenseServerInterface(Settings &settingsRef, QObject *parent)
    : QObject(parent), settings(settingsRef)
{
}

QByteArray LicenseServerInterface::requestValidatedToken(const QByteArray &token1, const QByteArray &token2)
{
    QSslSocket socket;

    QUrl url(settings.licenseServerUrl);

    if (!url.isValid())
    {
        qWarning() << "Invalid license server URL:" << settings.licenseServerUrl;
        return {};
    }

    QString host = url.host();
    int port = url.port(443);  // default to 443 if not specified

    socket.connectToHostEncrypted(host, port);

    if (!socket.waitForEncrypted(3000))
    {
        qWarning() << "TLS connection failed:" << socket.errorString();
        return {};
    }

    // Costruzione payload (token1 + token2)
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << quint32(token1.size()) << token1;
    stream << quint32(token2.size()) << token2;

    // Invia il payload
    socket.write(payload);

    if (!socket.waitForBytesWritten(2000))
    {
        qWarning() << "Failed to send tokens:" << socket.errorString();
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
