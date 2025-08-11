#include "LicenseServerInterface.h"
#include <QCoreApplication>
#include <QDir>
#include <QHostAddress>
#include <QUrl>
#include <QDataStream>
#include <QSslCertificate>
#include <QSslSocket>
#include <QFile>
#include <QDebug>

LicenseServerInterface::LicenseServerInterface(Settings &settingsRef, QObject *parent)
    : QObject(parent), settings(settingsRef)
{
    certPath = QDir(QCoreApplication::applicationDirPath()).filePath(settings.serverCertName);
    QFile certFile(certPath);
    if (!certFile.open(QIODevice::ReadOnly))
    {
        MYWARNING << __func__ << "() Cannot open certificate file:" << certPath;
        return;
    }

    QList<QSslCertificate> certs = QSslCertificate::fromDevice(&certFile, QSsl::Pem);
    certFile.close();

    if (certs.isEmpty())
    {
        MYWARNING << __func__ << "() Invalid certificate file";
        return;
    }

    // Imposta il certificato self-signed come CA fidata
    sslConf = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> caList = sslConf.caCertificates();
    caList.append(certs);
    sslConf.setCaCertificates(caList);
    sslConf.setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslConf.setProtocol(QSsl::TlsV1_2OrLater);
}

QByteArray LicenseServerInterface::requestValidatedToken(const QByteArray &activationKey, const QByteArray &incompleteToken)
{
    QSslSocket socket;

    QString host = settings.licenseServerSite;
    int port = settings.licenseServerPort;
    socket.setSslConfiguration(sslConf);

    // Gestisci errori SSL specifici
    QObject::connect(&socket, &QSslSocket::sslErrors,
                     [&socket](const QList<QSslError> &errors)
                     {
                         QList<QSslError> ignorableErrors;
                         for (const auto& error : errors)
                         {
                             // Ignora questi errori specifici
                             if (error.error() == QSslError::SelfSignedCertificate ||
                                 error.error() == QSslError::SelfSignedCertificateInChain ||
                                 error.error() == QSslError::HostNameMismatch)
                             {
                                 ignorableErrors.append(error);
                                 MYINFO << "Ignoring SSL error:" << error.errorString();
                             }
                             else
                             {
                                 MYINFO << "NOT ignoring SSL error:" << error.errorString();
                             }
                         }

                         // Ignora solo se tutti gli errori sono "conosciuti"
                         if (ignorableErrors.size() == errors.size())
                         {
                             socket.ignoreSslErrors(ignorableErrors);
                         }
                     });

    socket.connectToHostEncrypted(host, port);

    if (!socket.waitForEncrypted(5000))
    {
        MYWARNING << "TLS connection to " << host << " : " << port
                  << " failed:" << socket.errorString();
        return {};
    }

    MYINFO << "TLS connection successful!";


    QByteArray payload;
    payload.append('1');
    payload.append(activationKey);           
    payload.append(incompleteToken);         

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

