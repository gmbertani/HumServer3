#pragma once

#include <QObject>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QByteArray>
#include "settings.h"

class LicenseServerInterface : public QObject
{
    Q_OBJECT

public:
    explicit LicenseServerInterface(Settings &settingsRef, QObject *parent = nullptr);
    QByteArray requestValidatedToken(const QByteArray &activationKey, const QByteArray &incompleteToken);


private:
    QSslConfiguration sslConf;
    Settings &settings;
    QString certPath;
};
