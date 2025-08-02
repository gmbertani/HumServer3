#pragma once

#include <QObject>
#include <QSslSocket>
#include <QByteArray>
#include "settings.h"

class LicenseServerInterface : public QObject
{
    Q_OBJECT

public:
    explicit LicenseServerInterface(Settings &settingsRef, QObject *parent = nullptr);
    QByteArray requestValidatedToken(const QByteArray &activationKey, const QByteArray &incompleteToken);

private:
    Settings &settings;
};
