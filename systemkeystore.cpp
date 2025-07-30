#include "SystemKeyStore.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QUuid>
#include <QDebug>

SystemKeyStore::SystemKeyStore(QObject *parent)
    : QObject(parent)
{
    // Registry / config root: SOFTWARE\NUR\HumServer
    QCoreApplication::setOrganizationName("NUR");
    QCoreApplication::setApplicationName("HumServer");

    settings = new QSettings(QSettings::NativeFormat,
                             QSettings::UserScope,
                             QCoreApplication::organizationName(),
                             QCoreApplication::applicationName(),
                             this);
}

QString SystemKeyStore::getToken()
{
    QString token = readToken();

    if (token.isEmpty())
    {
        qDebug() << "[SystemKeyStore] hum_token is empty.";
    }
    else
    {
        qDebug() << "[SystemKeyStore] Loaded existing hum_token:" << token;
    }

    return token;
}

void SystemKeyStore::setToken(const QString &token)
{
    writeToken(token);
}

QString SystemKeyStore::readToken()
{
    return settings->value("hum_token").toString();
}

void SystemKeyStore::writeToken(const QString &token)
{
    settings->setValue("hum_token", token);
    settings->sync();
}

QString SystemKeyStore::getFingerprint()
{
    QByteArray hwInfo;

    // MAC addresses
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces)
    {
        if (!(iface.flags() & QNetworkInterface::IsLoopBack))
        {
            hwInfo.append(iface.hardwareAddress().toUtf8());
        }
    }

    // Hostname
    hwInfo.append(QHostInfo::localHostName().toUtf8());

    // Hash hardware info to generate UUID-like string
    QByteArray hash = QCryptographicHash::hash(hwInfo, QCryptographicHash::Sha256);
    QUuid uuid = QUuid::fromRfc4122(hash.left(16));

    return uuid.toString(QUuid::WithoutBraces);  // Ex: "5a1e4413-ef3c-493e-9c9f-d4b6a1f91aa1"
}
