#include "SystemKeyStore.h"
#include "settings.h"
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
    // Registry / config root: HKEY_CURRENT_USER\SOFTWARE\NUR\HumServer
    QCoreApplication::setOrganizationName("NUR");
    QCoreApplication::setApplicationName("HumServer");

    registry = new QSettings(QSettings::NativeFormat,
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
        MYDEBUG << "[SystemKeyStore] hum_token is empty.";
    }
    else
    {
        MYDEBUG << "[SystemKeyStore] Loaded existing hum_token:" << token;
    }

    return  QByteArray::fromHex(token);
}

void SystemKeyStore::createTempToken(QString& ctrlID)
{
    QByteArray utf8 = ctrlID.toUtf8();
    tempToken.setControllerID(utf8.constData());
    tempToken.setCheckTime(QDate::currentDate());
    tempToken.setFingerprint(getFingerprint());
    tempToken.setValidatedKey(QByteArray(32, '\0'));
}

bool SystemKeyStore::isTokenStillValid(QString ctrlID)
{
    QDate today = QDate::currentDate();
    QByteArray fingerprint = getFingerprint();
    bool valid = false;

    QString storedFingerprint = tempToken.getFingerprint();
    if(storedFingerprint == fingerprint)
    {
        QString storedControllerID = tempToken.getControllerID().trimmed();
        if(storedControllerID == ctrlID)
        {
            QDate checkTime = tempToken.getCheckTime();
            if(checkTime > today)
            {
                valid = true;
            }
        }
    }

    return valid;
}

bool SystemKeyStore::isTokenExpired(QString& ctrlID)
{
    QDate today = QDate::currentDate();
    QByteArray fingerprint = getFingerprint();
    bool expired = false;

    if(tempToken.getFingerprint() == fingerprint)
    {
        if(tempToken.getControllerID() == ctrlID)
        {
            if(tempToken.getCheckTime() <= today)
            {
                //time expired but controller and PC are ok
                expired = true;
            }
        }
    }

    //returns false when token is not valid at all
    return expired;
}



void SystemKeyStore::setToken(const QByteArray &token)
{
    writeToken(token);
}

void SystemKeyStore::writeToken(const QByteArray &token)
{
    //il token scritto nel registry è una stringa hex
    //per cui ad ogni byte corrispondono due caratteri

    registry->setValue("hum_token", token.toHex());
    registry->sync();
}

QByteArray SystemKeyStore::readToken()
{
    //per leggere il token occorre convertire la stringa hex di 176 caratteri
    //in un QByteArray di 88 bytes
    QByteArray hexToken = registry->value("hum_token").toByteArray();
    tempToken = HumToken::fromByteArray(QByteArray::fromHex(hexToken));
    return tempToken.toByteArray();
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
