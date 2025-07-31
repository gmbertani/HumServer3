#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDate>
#include "humtoken.h"


class SystemKeyStore : public QObject
{
    Q_OBJECT

public:
    explicit SystemKeyStore(QObject *parent = nullptr);

    QByteArray getToken();
    void createTemporaryToken(QString& ctrlID);
    void setToken(const QByteArray &token);
    QByteArray getTempToken() const
    {
        return tempToken.toByteArray();
    }

private:
    QSettings *settings;
    HumToken tempToken;

    QByteArray readToken();
    void writeToken(const QByteArray &token);
    QByteArray getFingerprint();  // UUID derived from hardware identity


};
