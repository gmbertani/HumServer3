#include "SystemKeyStore.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QUuid>
#include <QDebug>
#include <QtGlobal>

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

    tempToken = HumToken();
}

QByteArray SystemKeyStore::getToken()
{
    QByteArray token = readToken();

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

void SystemKeyStore::createTemporaryToken(QString& ctrlID)
{
    QByteArray utf8 = ctrlID.toUtf8();
    tempToken.setControllerID(utf8.constData());
    tempToken.setCheckTime( QDate::currentDate() );
    tempToken.setFingerprint(getFingerprint());
}


void SystemKeyStore::setToken(const QByteArray &token)
{
    writeToken(token);
}

QByteArray SystemKeyStore::readToken()
{
    QByteArray token = settings->value("hum_token").toByteArray();
    tempToken = HumToken::fromByteArray(token);
    return token;
}

void SystemKeyStore::writeToken(const QByteArray &token)
{
    tempToken = HumToken::fromByteArray(token);
    settings->setValue("hum_token", token);
    settings->sync();
}

QByteArray SystemKeyStore::getFingerprint()
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
    return QCryptographicHash::hash(hwInfo, QCryptographicHash::Sha256);
}
